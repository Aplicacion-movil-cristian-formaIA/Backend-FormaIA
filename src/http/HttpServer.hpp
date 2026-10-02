#pragma once
#include <memory>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "http/Router.hpp"
#include "utils/Logger.hpp"

namespace formaia::http {

namespace beast = boost::beast;
namespace beast_http = boost::beast::http;
namespace asio = boost::asio;
using tcp = boost::asio::ip::tcp;

// Servidor HTTP async simple: acepta conexiones sobre el io_context
// compartido con el EventBus, así que las peticiones HTTP y el
// procesamiento de eventos usan el mismo pool de hilos (ver main.cpp,
// io_context.run() en N hilos). Cada conexión se maneja con
// operaciones async_* (accept/read/write), nunca bloqueando un hilo
// completo por cliente -esto es lo que hace que el servidor pueda
// atender miles de conexiones concurrentes con pocos hilos del sistema
// operativo, el mismo principio detrás de Node.js o de nginx.
class HttpServer : public std::enable_shared_from_this<HttpServer> {
public:
    HttpServer(asio::io_context& io, const std::string& host, unsigned short port, Router& router)
        : io_(io), acceptor_(io), router_(router) {
        tcp::endpoint endpoint(asio::ip::make_address(host), port);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
    }

    void run() { aceptar(); }

private:
    void aceptar() {
        auto self = shared_from_this();
        auto socket = std::make_shared<tcp::socket>(io_);
        acceptor_.async_accept(*socket, [this, self, socket](beast::error_code ec) {
            if (!ec) {
                manejarConexion(socket);
            } else {
                utils::Logger::error("Error al aceptar conexión: " + ec.message());
            }
            aceptar(); // seguimos aceptando la siguiente conexión
        });
    }

    void manejarConexion(std::shared_ptr<tcp::socket> socket) {
        auto self = shared_from_this();
        auto buffer = std::make_shared<beast::flat_buffer>();
        auto req = std::make_shared<beast_http::request<beast_http::string_body>>();

        beast_http::async_read(*socket, *buffer, *req,
            [this, self, socket, buffer, req](beast::error_code ec, std::size_t) {
                if (ec) return; // conexión cerrada por el cliente, etc.

                auto res = std::make_shared<beast_http::response<beast_http::string_body>>();
                res->version(req->version());
                res->set(beast_http::field::server, "formaia-backend");
                res->set(beast_http::field::content_type, "application/json");
                res->set(beast_http::field::access_control_allow_origin, "*");
                res->set(beast_http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
                res->set(beast_http::field::access_control_allow_headers, "Content-Type, Authorization");

                // El navegador (ej. Flutter Web) envía una petición OPTIONS
                // de "preflight" antes de un POST con Content-Type: application/json.
                // No pasa por el Router: solo confirma que el origen está
                // permitido y corta aquí con 204 (sin cuerpo).
                if (req->method() == beast_http::verb::options) {
                    res->result(beast_http::status::no_content);
                    res->keep_alive(false);
                    res->prepare_payload();
                    beast_http::async_write(*socket, *res,
                        [socket, res](beast::error_code, std::size_t) {
                            beast::error_code ignore;
                            socket->shutdown(tcp::socket::shutdown_send, ignore);
                        });
                    return;
                }

                bool encontrada = router_.despachar(*req, *res);
                if (!encontrada) {
                    res->result(beast_http::status::not_found);
                    res->body() = R"({"error":"Ruta no encontrada"})";
                }
                res->keep_alive(false);
                res->prepare_payload();

                beast_http::async_write(*socket, *res,
                    [socket, res](beast::error_code, std::size_t) {
                        beast::error_code ignore;
                        socket->shutdown(tcp::socket::shutdown_send, ignore);
                    });
            });
    }

    asio::io_context& io_;
    tcp::acceptor acceptor_;
    Router& router_;
};

} // namespace formaia::http
