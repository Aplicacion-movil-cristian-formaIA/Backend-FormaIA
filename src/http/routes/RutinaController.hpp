#pragma once
#include "db/Database.hpp"
#include "domain/entities/Rutina.hpp"
#include "http/Router.hpp"
#include "orm/Repository.hpp"

namespace formaia::http::routes {

// Registra GET /api/rutinas/{id}. Devuelve la rutina y sus fases; las
// sesiones detalladas de cada fase se consultan aparte en un endpoint de
// listado paginado en una implementación completa (aquí se deja el
// esqueleto con la rutina + fases para no extender demasiado el ejemplo).
inline void registrarRutasRutina(Router& router, db::ConnectionPool& pool) {
    using namespace formaia::domain::entities;
    namespace bhttp = boost::beast::http;

    router.add(bhttp::verb::get, "/api/rutinas/{id}",
        [&pool](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            orm::Repository<Rutina> repoRutina(pool, Rutina::tabla(), Rutina::columnas());
            orm::Repository<Fase> repoFase(pool, Fase::tabla(), Fase::columnas());

            auto rutina = repoRutina.buscarPorId(ctx.params[0]);
            if (!rutina) {
                res.result(bhttp::status::not_found);
                res.body() = R"({"error":"Rutina no encontrada"})";
                return;
            }

            auto fases = repoFase.buscarTodosPor("rutina_id = :valor", rutina->id);

            nlohmann::json fasesJson = nlohmann::json::array();
            for (auto& f : fases) {
                fasesJson.push_back({
                    {"orden", f.orden},
                    {"nombre", f.nombre},
                    {"semana_inicio", f.semana_inicio},
                    {"semana_fin", f.semana_fin},
                    {"objetivo", f.objetivo},
                });
            }

            nlohmann::json out = {
                {"id", rutina->id},
                {"nombre", rutina->nombre},
                {"semanas_totales", rutina->semanas_totales},
                {"activa", rutina->activa},
                {"fases", fasesJson},
            };
            res.body() = out.dump();
        });

    // El cliente crea la solicitud y solo conoce su solicitud_id (la
    // respuesta de POST /api/solicitudes-ia no incluye rutina_id porque
    // en ese momento la rutina todavía no existe: se genera de forma
    // asíncrona). Este endpoint le permite, una vez que el polling a
    // GET /api/solicitudes-ia/{id} muestra estado = "generada", encontrar
    // la rutina resultante sin que el backend tenga que exponer su id
    // interno antes de tiempo.
    router.add(bhttp::verb::get, "/api/rutinas/por-solicitud/{id}",
        [&pool](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            orm::Repository<Rutina> repoRutina(pool, Rutina::tabla(), Rutina::columnas());
            auto rutinas = repoRutina.buscarTodosPor("solicitud_id = :valor", ctx.params[0]);
            if (rutinas.empty()) {
                res.result(bhttp::status::not_found);
                res.body() = R"({"error":"Aun no hay una rutina generada para esta solicitud"})";
                return;
            }
            nlohmann::json out = {{"rutina_id", rutinas.front().id}};
            res.body() = out.dump();
        });

    router.add(bhttp::verb::get, "/api/rutinas/activa/{usuario_id}",
        [&pool](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            orm::Repository<Rutina> repoRutina(pool, Rutina::tabla(), Rutina::columnas());
            auto rutinas = repoRutina.buscarTodosPor("usuario_id = :v1 AND activa = 1", ctx.params[0]);
            if (rutinas.empty()) {
                res.result(bhttp::status::not_found);
                res.body() = R"({"error":"No hay rutina activa"})";
                return;
            }
            nlohmann::json out = {{"rutina_id", rutinas.front().id}};
            res.body() = out.dump();
        });
}

} // namespace formaia::http::routes
