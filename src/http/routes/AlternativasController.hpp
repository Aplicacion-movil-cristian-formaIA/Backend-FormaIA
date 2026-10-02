#pragma once
#include <string>
#include "config/Config.hpp"
#include "http/Router.hpp"
#include "ia/GroqClient.hpp"
#include <nlohmann/json.hpp>

namespace formaia::http::routes {

inline void registrarRutasAlternativas(Router& router, const config::Config& cfg) {
    namespace bhttp = boost::beast::http;

    router.add(bhttp::verb::post, "/api/ia/alternativas",
        [&cfg](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string ejercicio = ctx.body.value("ejercicio", "");
            std::string motivo = ctx.body.value("motivo", "");

            if (ejercicio.empty() || motivo.empty()) {
                res.result(bhttp::status::bad_request);
                res.body() = R"({"error":"ejercicio y motivo son obligatorios"})";
                return;
            }

            try {
                // Instanciar un io_context temporal solo para instanciar la clase,
                // aunque chatSync creará el suyo propio para bloquearse.
                boost::asio::io_context dummy_io;
                ia::GroqClient groq(dummy_io, cfg);
                
                std::string prompt = 
                    "Eres un entrenador personal experto. Tu cliente necesita cambiar el ejercicio '" + ejercicio + 
                    "' porque '" + motivo + "'. Sugiere exactamente 3 ejercicios alternativos que trabajen "
                    "los mismos grupos musculares o cumplan el mismo objetivo.\n\n"
                    "Responde ESTRICTAMENTE con un array JSON en este formato sin texto adicional:\n"
                    "[\n"
                    "  {\"nombre\": \"Nombre 1\", \"razon\": \"Por qué sirve\"},\n"
                    "  {\"nombre\": \"Nombre 2\", \"razon\": \"Por qué sirve\"},\n"
                    "  {\"nombre\": \"Nombre 3\", \"razon\": \"Por qué sirve\"}\n"
                    "]";

                std::string groqRes = groq.chatSync(prompt, "");
                
                // Limpiar posible formato markdown de la respuesta de la IA (```json ... ```)
                size_t start = groqRes.find("[");
                size_t end = groqRes.rfind("]");
                if (start != std::string::npos && end != std::string::npos) {
                    groqRes = groqRes.substr(start, end - start + 1);
                }

                auto alternativasJson = nlohmann::json::parse(groqRes);

                res.result(bhttp::status::ok);
                res.body() = alternativasJson.dump();

            } catch (const std::exception& e) {
                res.result(bhttp::status::internal_server_error);
                res.body() = "{\"error\":\"Error interno de IA: " + std::string(e.what()) + "\"}";
            }
        });

    router.add(bhttp::verb::post, "/api/ia/adaptar-sesion",
        [&cfg](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string motivo = ctx.body.value("motivo", "");
            nlohmann::json ejerciciosArr;
            if (ctx.body.contains("ejercicios")) {
                ejerciciosArr = ctx.body["ejercicios"];
            }

            if (motivo.empty() || ejerciciosArr.empty()) {
                res.result(bhttp::status::bad_request);
                res.body() = R"({"error":"motivo y ejercicios son obligatorios"})";
                return;
            }

            try {
                boost::asio::io_context dummy_io;
                ia::GroqClient groq(dummy_io, cfg);
                
                std::string prompt = 
                    "Eres la IA de Freeletics/Fitbod. Tu cliente tiene la siguiente sesión de entrenamiento programada:\n"
                    + ejerciciosArr.dump() + "\n\n"
                    "Pero hoy ocurre esto: '" + motivo + "'.\n"
                    "Por favor, ADAPTA la sesión completa para ajustarse a esta restricción (cambia ejercicios, series o omite cosas si es necesario de forma inteligente).\n\n"
                    "Responde ESTRICTAMENTE con un array JSON de la nueva rutina adaptada en este formato:\n"
                    "[\n"
                    "  {\"nombre\": \"Nombre Ejercicio 1\", \"series\": 3, \"razon\": \"Por qué lo elegiste\"}\n"
                    "]";

                std::string groqRes = groq.chatSync(prompt, "");
                
                size_t start = groqRes.find("[");
                size_t end = groqRes.rfind("]");
                if (start != std::string::npos && end != std::string::npos) {
                    groqRes = groqRes.substr(start, end - start + 1);
                }

                auto adaptadaJson = nlohmann::json::parse(groqRes);

                res.result(bhttp::status::ok);
                res.body() = adaptadaJson.dump();

            } catch (const std::exception& e) {
                res.result(bhttp::status::internal_server_error);
                res.body() = "{\"error\":\"Error interno de IA Adaptativa: " + std::string(e.what()) + "\"}";
            }
        });
}

} // namespace formaia::http::routes
