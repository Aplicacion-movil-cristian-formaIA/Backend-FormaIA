#pragma once
#include "core/EventBus.hpp"
#include "db/Database.hpp"
#include "domain/entities/PerfilFisico.hpp"
#include "domain/entities/SolicitudIA.hpp"
#include "domain/events/DomainEvents.hpp"
#include "http/Router.hpp"
#include "orm/Repository.hpp"
#include "security/Crypto.hpp"
#include "utils/Uuid.hpp"

namespace formaia::http::routes {

// Registra POST /api/solicitudes-ia (RF-06/HU-04) y GET /api/solicitudes-ia/{id}
// (para que el cliente haga polling del resultado; ver nota sobre
// WebSocket en NotificationHandler para una alternativa push).
//
// Este endpoint es el ejemplo más claro de la arquitectura orientada a
// eventos: el handler HTTP NO llama a Groq, NO arma la rutina, NO la
// guarda. Solo inserta la fila inicial y publica un evento. Todo lo
// demás ocurre de forma asíncrona en la cadena de handlers (ver
// ia/IAOrchestratorHandler.hpp, ia/MetaSeguridadHandler.hpp,
// rutinas/RutinaBuilderHandler.hpp, rutinas/PersistenceHandler.hpp), y
// el hilo que atendió esta petición HTTP queda libre de inmediato.
inline void registrarRutasSolicitud(Router& router,
                                     core::EventBus& bus,
                                     db::ConnectionPool& pool,
                                     security::Crypto& crypto) {
    using namespace formaia::domain::entities;
    namespace bhttp = boost::beast::http;

    router.add(bhttp::verb::post, "/api/solicitudes-ia",
        [&bus, &pool, &crypto](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string usuarioId = ctx.body.value("usuario_id", "");
            std::string texto = ctx.body.value("texto", "");

            if (usuarioId.empty() || texto.empty()) {
                res.result(bhttp::status::bad_request);
                res.body() = R"({"error":"usuario_id y texto son obligatorios"})";
                return;
            }

            SolicitudIA s;
            s.id = utils::newUuid();
            s.usuario_id = usuarioId;
            s.texto_usuario_cifrado_b64 = crypto.cifrar(texto);
            s.estado = "aclaracion";

            orm::Repository<SolicitudIA> repo(pool, SolicitudIA::tabla(), SolicitudIA::columnas());
            repo.insertar(s);

            domain::events::SolicitudIACreada evento;
            evento.solicitud_id = s.id;
            evento.usuario_id = usuarioId;
            evento.texto_usuario = texto; // en claro solo dentro del proceso, nunca se persiste así
            evento.correlacion_id = s.id;

            // Extraer PerfilFisico para que la IA se adapte al usuario
            orm::Repository<formaia::domain::entities::PerfilFisico> repoP(pool, formaia::domain::entities::PerfilFisico::tabla(), formaia::domain::entities::PerfilFisico::columnas());
            auto perfil = repoP.buscarPorId(usuarioId);
            if (perfil) {
                nlohmann::json pj = {
                    {"sexo", perfil->sexo},
                    {"nivel", perfil->nivel},
                    {"dias_semana", perfil->dias_semana},
                    {"minutos_sesion", perfil->minutos_sesion}
                };
                try {
                    pj["equipamiento"] = nlohmann::json::parse(perfil->equipamiento_json);
                } catch(...) {}
                evento.perfil_fisico_json = pj.dump();
            }

            bus.publish(evento); // <-- publica y retorna al instante, no espera a la IA

            nlohmann::json out = {
                {"id", s.id},
                {"estado", "procesando"},
                {"mensaje", "Tu rutina se está generando. Consulta GET /api/solicitudes-ia/" + s.id}
            };
            res.result(bhttp::status::accepted); // 202: aceptado, procesando en segundo plano
            res.body() = out.dump();
        });

    router.add(bhttp::verb::get, "/api/solicitudes-ia/{id}",
        [&pool, &crypto](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            orm::Repository<SolicitudIA> repo(pool, SolicitudIA::tabla(), SolicitudIA::columnas());
            auto s = repo.buscarPorId(ctx.params[0]);
            if (!s) {
                res.result(bhttp::status::not_found);
                res.body() = R"({"error":"Solicitud no encontrada"})";
                return;
            }
            nlohmann::json out = {
                {"id", s->id},
                {"estado", s->estado},
                {"motivo_rechazo", s->motivo_rechazo},
                {"referente_detectado", s->referente_detectado},
            };
            res.body() = out.dump();
        });
}

} // namespace formaia::http::routes
