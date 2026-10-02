#pragma once
#include <algorithm>
#include <ctime>
#include "config/Config.hpp"
#include "db/Database.hpp"
#include "domain/entities/PerfilFisico.hpp"
#include "domain/entities/Usuario.hpp"
#include "http/Router.hpp"
#include "orm/Repository.hpp"
#include "security/Crypto.hpp"
#include "security/PasswordHasher.hpp"
#include "utils/Uuid.hpp"

namespace formaia::http::routes {

// Registra POST /api/usuarios (RF-01 / HU-01) y POST /api/usuarios/{id}/perfil
// (RF-03 / HU-02). Es intencionalmente simple (sin JWT todavía) para que el
// flujo de "crear rutina con IA" se pueda probar de punta a punta; añadir
// autenticación real es una extensión directa sobre este mismo controlador.
inline void registrarRutasUsuario(Router& router,
                                   db::ConnectionPool& pool,
                                   security::Crypto& crypto,
                                   const config::Config& cfg) {
    using namespace formaia::domain::entities;
    namespace bhttp = boost::beast::http;

    router.add(bhttp::verb::post, "/api/usuarios",
        [&pool, &crypto, &cfg](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string email = ctx.body.value("email", "");
            std::string password = ctx.body.value("password", "");
            std::string fechaNacimiento = ctx.body.value("fecha_nacimiento", ""); // YYYY-MM-DD

            if (email.empty() || password.size() < 8 || fechaNacimiento.empty()) {
                res.result(bhttp::status::bad_request);
                res.body() = R"({"error":"email, password (min 8) y fecha_nacimiento son obligatorios"})";
                return;
            }

            std::string emailMin = email;
            std::transform(emailMin.begin(), emailMin.end(), emailMin.begin(), ::tolower);

            Usuario u;
            u.id = utils::newUuid();
            u.email_cifrado_b64 = crypto.cifrar(emailMin);
            u.email_idx = security::Crypto::hmacIndice(emailMin, cfg.field_encryption_key_base64);
            u.password_hash = security::PasswordHasher::hash(password);
            u.proveedor_auth = "email";
            u.fecha_nacimiento_cifrada_b64 = crypto.cifrar(fechaNacimiento);
            
            // Asignar fecha actual simulando que acaba de aceptar
            time_t now = time(nullptr);
            char buf[20];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
            u.acepto_terminos_en = buf;

            try {
                orm::Repository<Usuario> repo(pool, Usuario::tabla(), Usuario::columnas());
                repo.insertar(u);
                nlohmann::json out = {{"id", u.id}, {"email", emailMin}};
                res.result(bhttp::status::created);
                res.body() = out.dump();
            } catch (const std::exception& e) {
                // Si el correo ya existe, da error de llave duplicada
                std::string errStr = e.what();
                if (errStr.find("Duplicate entry") != std::string::npos) {
                    res.result(bhttp::status::conflict);
                    res.body() = R"({"error":"El correo ya está registrado. Por favor, inicia sesión."})";
                } else {
                    res.result(bhttp::status::internal_server_error);
                    res.body() = R"({"error":"Error interno al registrar usuario"})";
                }
            }
        });

    // Endpoint provisional para INICIAR SESIÓN (HU-01)
    router.add(bhttp::verb::post, "/api/login",
        [&pool, &cfg](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string email = ctx.body.value("email", "");
            std::string password = ctx.body.value("password", "");

            if (email.empty() || password.empty()) {
                res.result(bhttp::status::bad_request);
                res.body() = R"({"error":"email y password son obligatorios"})";
                return;
            }

            std::string emailMin = email;
            std::transform(emailMin.begin(), emailMin.end(), emailMin.begin(), ::tolower);
            std::string emailIdx = security::Crypto::hmacIndice(emailMin, cfg.field_encryption_key_base64);

            try {
                auto conn = pool.acquire();
                auto result = conn->sql("SELECT id, password_hash FROM usuario WHERE email_idx = ?")
                                  .bind(emailIdx).execute();
                
                auto row = result.fetchOne();
                if (!row) {
                    res.result(bhttp::status::unauthorized);
                    res.body() = R"({"error":"Credenciales incorrectas"})";
                    return;
                }

                std::string userId = static_cast<std::string>(row[0]);
                std::string dbHash = row[1].isNull() ? "" : static_cast<std::string>(row[1]);

                if (security::PasswordHasher::verificar(password, dbHash)) {
                    nlohmann::json out = {{"id", userId}, {"email", emailMin}};
                    res.result(bhttp::status::ok);
                    res.body() = out.dump();
                } else {
                    res.result(bhttp::status::unauthorized);
                    res.body() = R"({"error":"Credenciales incorrectas"})";
                }
            } catch (const std::exception& e) {
                res.result(bhttp::status::internal_server_error);
                res.body() = R"({"error":"Error interno en el login"})";
            }
        });

    router.add(bhttp::verb::post, "/api/usuarios/{id}/perfil",
        [&pool, &crypto](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string usuarioId = ctx.params[0];

            PerfilFisico p;
            p.usuario_id = usuarioId;
            p.sexo = ctx.body.value("sexo", "prefiero_no_decir");
            p.nivel = ctx.body.value("nivel", "principiante");
            p.dias_semana = ctx.body.value("dias_semana", 3);
            p.minutos_sesion = ctx.body.value("minutos_sesion", 30);
            if (ctx.body.contains("equipamiento")) p.equipamiento_json = ctx.body["equipamiento"].dump();

            double estatura = ctx.body.value("estatura_cm", 0.0);
            double peso = ctx.body.value("peso_kg", 0.0);
            p.estatura_cm_cifrada_b64 = crypto.cifrar(std::to_string(estatura));
            p.peso_kg_cifrada_b64 = crypto.cifrar(std::to_string(peso));

            orm::Repository<PerfilFisico> repo(pool, PerfilFisico::tabla(), PerfilFisico::columnas());
            auto existe = repo.buscarPorId(usuarioId);
            try {
                if (existe) {
                    repo.actualizar(p, usuarioId);
                } else {
                    repo.insertar(p);
                }
            } catch (const std::exception& e) {
                res.result(bhttp::status::internal_server_error);
                res.body() = "{\"error\":\"" + std::string(e.what()) + "\"}";
                return;
            }

            res.result(bhttp::status::ok);
            res.body() = R"({"ok":true})";
        });

    router.add(bhttp::verb::get, "/api/usuarios/{id}/perfil",
        [&pool, &crypto](HttpContext& ctx, bhttp::response<bhttp::string_body>& res) {
            std::string usuarioId = ctx.params[0];
            orm::Repository<PerfilFisico> repo(pool, PerfilFisico::tabla(), PerfilFisico::columnas());
            auto perfil = repo.buscarPorId(usuarioId);
            if (!perfil) {
                res.result(bhttp::status::not_found);
                res.body() = R"({"error":"Perfil no encontrado"})";
                return;
            }
            
            nlohmann::json out;
            out["usuario_id"] = perfil->usuario_id;
            out["sexo"] = perfil->sexo;
            out["nivel"] = perfil->nivel;
            out["dias_semana"] = perfil->dias_semana;
            out["minutos_sesion"] = perfil->minutos_sesion;
            try {
                out["equipamiento"] = nlohmann::json::parse(perfil->equipamiento_json);
            } catch(...) {
                out["equipamiento"] = nlohmann::json::array();
            }
            
            try {
                std::string est = crypto.descifrar(perfil->estatura_cm_cifrada_b64);
                std::string pes = crypto.descifrar(perfil->peso_kg_cifrada_b64);
                out["estatura_cm"] = std::stod(est);
                out["peso_kg"] = std::stod(pes);
            } catch(...) {
                out["estatura_cm"] = 0.0;
                out["peso_kg"] = 0.0;
            }

            res.result(bhttp::status::ok);
            res.set(bhttp::field::content_type, "application/json");
            res.body() = out.dump();
        });
}

} // namespace formaia::http::routes
