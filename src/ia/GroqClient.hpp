#pragma once
#include <functional>
#include <memory>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

#include "config/Config.hpp"
#include "utils/Logger.hpp"

namespace formaia::ia {

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

// GroqClient llama al endpoint de "chat completions" de Groq
// (https://api.groq.com/openai/v1/chat/completions, API compatible con
// OpenAI) de forma completamente asíncrona: se le pasa un callback que
// se invoca cuando la respuesta llega, y el hilo que llamó a chatAsync()
// NUNCA se bloquea esperando la red. Esto es lo que permite que el
// EventBus siga despachando otros eventos mientras Groq responde.
//
// Se le pide al modelo que devuelva SIEMPRE un JSON con un esquema fijo
// (RNF-08): así el backend puede parsear la respuesta sin ambigüedad.
class GroqClient : public std::enable_shared_from_this<GroqClient> {
public:
    using Callback = std::function<void(bool exito, const std::string& mensajeError, nlohmann::json respuesta)>;

    GroqClient(asio::io_context& io, const config::Config& cfg)
        : io_(io), cfg_(cfg), ssl_ctx_(ssl::context::tlsv12_client) {
        ssl_ctx_.set_default_verify_paths();
        ssl_ctx_.set_verify_mode(ssl::verify_peer);
    }

    // system_prompt: instrucciones de rol + el JSON schema esperado.
    // user_prompt: el mensaje del usuario (o un resumen ya construido).
    void chatAsync(const std::string& system_prompt,
                    const std::string& user_prompt,
                    Callback callback) {
        auto self = shared_from_this();

        auto resolver = std::make_shared<tcp::resolver>(io_);
        auto stream = std::make_shared<beast::ssl_stream<beast::tcp_stream>>(io_, ssl_ctx_);

        if (!SSL_set_tlsext_host_name(stream->native_handle(), cfg_.groq_api_host.c_str())) {
            callback(false, "No se pudo configurar SNI para TLS", {});
            return;
        }

        nlohmann::json body = {
            {"model", cfg_.groq_model},
            {"temperature", 0.3},
            {"response_format", {{"type", "json_object"}}},
            {"messages", nlohmann::json::array({
                {{"role", "system"}, {"content", system_prompt}},
                {{"role", "user"}, {"content", user_prompt}}
            })}
        };
        auto bodyStr = std::make_shared<std::string>(body.dump());

        auto req = std::make_shared<http::request<http::string_body>>();
        req->method(http::verb::post);
        req->target("/openai/v1/chat/completions");
        req->version(11);
        req->set(http::field::host, cfg_.groq_api_host);
        req->set(http::field::authorization, "Bearer " + cfg_.groq_api_key);
        req->set(http::field::content_type, "application/json");
        req->set(http::field::user_agent, "formaia-backend/0.1 (+groq)");
        req->body() = *bodyStr;
        req->prepare_payload();

        beast::get_lowest_layer(*stream).expires_after(
            std::chrono::milliseconds(cfg_.groq_timeout_ms));

        resolver->async_resolve(
            cfg_.groq_api_host, "443",
            [self, resolver, stream, req, callback](beast::error_code ec, tcp::resolver::results_type results) {
                if (ec) { callback(false, "DNS: " + ec.message(), {}); return; }

                beast::get_lowest_layer(*stream).async_connect(
                    results,
                    [self, stream, req, callback](beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
                        if (ec) { callback(false, "Conexión TCP: " + ec.message(), {}); return; }

                        stream->async_handshake(
                            ssl::stream_base::client,
                            [self, stream, req, callback](beast::error_code ec) {
                                if (ec) { callback(false, "Handshake TLS: " + ec.message(), {}); return; }

                                http::async_write(*stream, *req,
                                    [self, stream, req, callback](beast::error_code ec, std::size_t) {
                                        if (ec) { callback(false, "Escritura HTTP: " + ec.message(), {}); return; }

                                        auto buffer = std::make_shared<beast::flat_buffer>();
                                        auto res = std::make_shared<http::response<http::string_body>>();

                                        http::async_read(*stream, *buffer, *res,
                                            [self, stream, buffer, res, callback](beast::error_code ec, std::size_t) {
                                                self->manejarRespuesta(ec, *res, callback);
                                                // Cierre best-effort; no bloqueamos si el servidor ya cerró.
                                                beast::error_code ignore;
                                                stream->shutdown(ignore);
                                            });
                                    });
                            });
                    });
            });
    }

private:
    void manejarRespuesta(beast::error_code ec,
                           http::response<http::string_body>& res,
                           const Callback& callback) {
        if (ec && ec != http::error::end_of_stream) {
            callback(false, "Lectura HTTP: " + ec.message(), {});
            return;
        }

        if (res.result_int() < 200 || res.result_int() >= 300) {
            utils::Logger::error("Groq respondió HTTP " + std::to_string(res.result_int()) +
                                  ": " + res.body());
            callback(false, "Groq HTTP " + std::to_string(res.result_int()), {});
            return;
        }

        try {
            auto json = nlohmann::json::parse(res.body());
            std::string contenido = json["choices"][0]["message"]["content"].get<std::string>();
            auto contenidoJson = nlohmann::json::parse(contenido);
            callback(true, "", contenidoJson);
        } catch (const std::exception& ex) {
            utils::Logger::error(std::string("No se pudo parsear la respuesta de Groq: ") + ex.what());
            callback(false, std::string("Parseo de respuesta: ") + ex.what(), {});
        }
    }

    asio::io_context& io_;
    const config::Config& cfg_;
    ssl::context ssl_ctx_;
};

} // namespace formaia::ia
