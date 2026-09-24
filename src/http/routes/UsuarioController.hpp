#pragma once
#include <algorithm>
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

            orm::Repository<Usuario> repo(pool, Usuario::tabla(), Usuario::columnas());
            repo.insertar(u);

            nlohmann::json out = {{"id", u.id}, {"email", emailMin}};
            res.result(bhttp::status::created);
            res.body() = out.dump();
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
            // Nota: para simplificar el ejemplo se usa insertar(); en un
            // registro real conviene "insertar si no existe, si no
            // actualizar" (UPSERT), ya que usuario_id es la clave primaria.
            repo.insertar(p);

            res.result(bhttp::status::ok);
            res.body() = R"({"ok":true})";
        });
}

} // namespace formaia::http::routes
