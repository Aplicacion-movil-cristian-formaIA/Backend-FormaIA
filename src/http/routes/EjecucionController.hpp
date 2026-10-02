#pragma once
#include <string>
#include "config/Config.hpp"
#include "db/Database.hpp"
#include "domain/entities/Entrenamiento.hpp"
#include "domain/entities/RegistroSerie.hpp"
#include "http/Router.hpp"
#include "orm/Repository.hpp"
#include "utils/Calculadora1RM.hpp"
#include "utils/Uuid.hpp"

namespace formaia::http::routes {

inline void registrarRutasEjecucion(Router& router, db::ConnectionPool& pool) {
    using namespace formaia::domain::entities;
    namespace bhttp = boost::beast::http;

    router.add(bhttp::verb::post, "/api/sesiones",
        [&pool](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string usuarioId = ctx.body.value("usuario_id", "");
            std::string sesionPlanId = ctx.body.value("sesion_plan_id", "");
            
            if (usuarioId.empty() || sesionPlanId.empty()) {
                res.result(bhttp::status::bad_request);
                res.body() = R"({"error":"usuario_id y sesion_plan_id son obligatorios"})";
                return;
            }

            try {
                // 1. Crear el Entrenamiento
                Entrenamiento e;
                e.id = utils::newUuid();
                e.usuario_id = usuarioId;
                e.sesion_plan_id = sesionPlanId;
                
                time_t now = time(nullptr);
                char buf[20];
                strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
                e.iniciado_en = buf;
                e.finalizado_en = buf; // Asumimos que envía al finalizar
                
                e.completado = ctx.body.value("completado", true);
                if (ctx.body.contains("rpe")) {
                    e.rpe = ctx.body.value("rpe", 5);
                }

                orm::Repository<Entrenamiento> repoE(pool, Entrenamiento::tabla(), Entrenamiento::columnas());
                repoE.insertar(e);

                // 2. Guardar las series reales
                if (ctx.body.contains("series") && ctx.body["series"].is_array()) {
                    orm::Repository<RegistroSerie> repoS(pool, RegistroSerie::tabla(), RegistroSerie::columnas());
                    
                    for (auto& s : ctx.body["series"]) {
                        RegistroSerie rs;
                        rs.id = utils::newUuid();
                        rs.entrenamiento_id = e.id;
                        rs.ejercicio_id = s.value("ejercicio_id", "default-id");
                        rs.numero_serie = s.value("numero_serie", 1);
                        
                        if (s.contains("reps")) rs.repeticiones_hechas = s.value("reps", 0);
                        if (s.contains("peso")) rs.carga_kg = s.value("peso", 0.0);

                        repoS.insertar(rs);
                    }
                }

                // Calcular 1RM para cada ejercicio y generar notificaciones
                nlohmann::json notificaciones = nlohmann::json::array();
                if (ctx.body.contains("series") && ctx.body["series"].is_array()) {
                    std::map<std::string, double> max1RM;
                    for (auto& s : ctx.body["series"]) {
                        std::string ej = s.value("ejercicio_id", "Desconocido");
                        int reps = s.value("reps", 0);
                        double peso = s.value("peso", 0.0);
                        if (reps > 0 && peso > 0) {
                            double orm = utils::Calculadora1RM::calcularBrzycki(peso, reps);
                            if (max1RM.find(ej) == max1RM.end() || orm > max1RM[ej]) {
                                max1RM[ej] = orm;
                            }
                        }
                    }
                    for (auto const& [ej, rm] : max1RM) {
                        std::string mensaje = "¡Nuevo récord en " + ej + "! Tu 1RM estimado es " + std::to_string(static_cast<int>(rm)) + " kg.";
                        notificaciones.push_back(mensaje);
                    }
                }

                nlohmann::json resJson;
                resJson["ok"] = true;
                resJson["mensaje"] = "Entrenamiento registrado exitosamente";
                if (!notificaciones.empty()) {
                    resJson["notificaciones"] = notificaciones;
                }

                res.result(bhttp::status::created);
                res.body() = resJson.dump();

            } catch (const std::exception& e) {
                res.result(bhttp::status::internal_server_error);
                res.body() = "{\"error\":\"Error interno al registrar sesión: " + std::string(e.what()) + "\"}";
            }
        });
}

} // namespace formaia::http::routes
