#pragma once
#include <string>
#include "config/Config.hpp"
#include "core/EventBus.hpp"
#include "domain/events/DomainEvents.hpp"
#include "http/Router.hpp"
#include "ia/GroqClient.hpp"
#include <nlohmann/json.hpp>

namespace formaia::http::routes {

inline void registrarRutasEvolucion(Router& router, core::EventBus& bus, const config::Config& cfg) {
    namespace bhttp = boost::beast::http;

    router.add(bhttp::verb::post, "/api/sesiones/{id}/completar",
        [&bus](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string sesionId = ctx.params[0];
            std::string usuarioId = ctx.body.value("usuario_id", "");
            int rpe = ctx.body.value("rpe", 5); // default RPE

            domain::events::SesionCompletada evento;
            evento.sesion_id = sesionId;
            evento.usuario_id = usuarioId;
            evento.rpe = rpe;

            bus.publish(evento);

            res.result(bhttp::status::ok);
            res.body() = R"({"ok":true, "mensaje":"Sesion completada, feedback registrado"})";
        });

    router.add(bhttp::verb::get, "/api/ia/evolucionar/{usuario_id}",
        [&cfg](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string usuarioId = ctx.params[0];

            try {
                // Instanciar un io_context temporal
                boost::asio::io_context dummy_io;
                ia::GroqClient groq(dummy_io, cfg);
                
                std::string prompt = 
                    "Eres la IA de un entrenador virtual premium. El usuario ha terminado su primera semana de entrenamiento.\n"
                    "Imagina que completó sus ejercicios con buena técnica pero el RPE (esfuerzo) fue bajo en ejercicios de pecho y piernas, "
                    "indicando que está listo para levantar más peso.\n\n"
                    "Genera un resumen de la semana con ajustes de sobrecarga progresiva.\n"
                    "Responde ESTRICTAMENTE con un objeto JSON en este formato:\n"
                    "{\n"
                    "  \"mensaje_motivacional\": \"¡Gran trabajo esta semana! He notado que te sobra energía en ciertos ejercicios.\",\n"
                    "  \"ajustes\": [\n"
                    "    {\"ejercicio\": \"Press de Banca\", \"incremento\": \"+2.5 kg\", \"razon\": \"RPE bajo, buena técnica\"},\n"
                    "    {\"ejercicio\": \"Sentadilla\", \"incremento\": \"+5.0 kg\", \"razon\": \"Completaste todas las series fácil\"}\n"
                    "  ]\n"
                    "}";

                std::string groqRes = groq.chatSync(prompt, "");
                
                size_t start = groqRes.find("{");
                size_t end = groqRes.rfind("}");
                if (start != std::string::npos && end != std::string::npos) {
                    groqRes = groqRes.substr(start, end - start + 1);
                }

                auto jsonRes = nlohmann::json::parse(groqRes);

                res.result(bhttp::status::ok);
                res.body() = jsonRes.dump();

            } catch (const std::exception& e) {
                res.result(bhttp::status::internal_server_error);
                res.body() = "{\"error\":\"Error interno de IA (Evolucion): " + std::string(e.what()) + "\"}";
            }
        });
}

} // namespace formaia::http::routes
