#pragma once
#include <string>
#include <vector>
#include "orm/Column.hpp"

namespace formaia::domain::entities {

struct Usuario {
    std::string id;
    std::string email_cifrado_b64;   // ya cifrado por Crypto antes de llegar aquí
    std::string email_idx;           // HMAC-SHA-256(email en minúsculas)
    std::string password_hash;
    std::string proveedor_auth = "email";
    std::string fecha_nacimiento_cifrada_b64;
    std::string rol = "usuario";
    std::string acepto_terminos_en;  // Fecha en formato YYYY-MM-DD HH:MM:SS

    static std::string tabla() { return "usuario"; }

    static std::vector<orm::Column<Usuario>> columnas() {
        using C = orm::Column<Usuario>;
        return {
            C{"id", true,
              [](const Usuario& u) { return mysqlx::Value(u.id); },
              [](Usuario& u, const mysqlx::Value& v) { u.id = formaia::orm::safe_get_string(v); }},
            C{"email", false,
              [](const Usuario& u) { return mysqlx::Value(u.email_cifrado_b64); },
              [](Usuario& u, const mysqlx::Value& v) { u.email_cifrado_b64 = formaia::orm::safe_get_string(v); }},
            C{"email_idx", false,
              [](const Usuario& u) { return mysqlx::Value(u.email_idx); },
              [](Usuario& u, const mysqlx::Value& v) { u.email_idx = formaia::orm::safe_get_string(v); }},
            C{"password_hash", false,
              [](const Usuario& u) { return mysqlx::Value(u.password_hash); },
              [](Usuario& u, const mysqlx::Value& v) { u.password_hash = formaia::orm::safe_get_string(v); }},
            C{"proveedor_auth", false,
              [](const Usuario& u) { return mysqlx::Value(u.proveedor_auth); },
              [](Usuario& u, const mysqlx::Value& v) { u.proveedor_auth = formaia::orm::safe_get_string(v); }},
            C{"fecha_nacimiento", false,
              [](const Usuario& u) { return mysqlx::Value(u.fecha_nacimiento_cifrada_b64); },
              [](Usuario& u, const mysqlx::Value& v) { u.fecha_nacimiento_cifrada_b64 = formaia::orm::safe_get_string(v); }},
            C{"rol", false,
              [](const Usuario& u) { return mysqlx::Value(u.rol); },
              [](Usuario& u, const mysqlx::Value& v) { u.rol = formaia::orm::safe_get_string(v); }},
            C{"acepto_terminos_en", false,
              [](const Usuario& u) { return mysqlx::Value(u.acepto_terminos_en); },
              [](Usuario& u, const mysqlx::Value& v) { u.acepto_terminos_en = formaia::orm::safe_get_string(v); }},
        };
    }
};

} // namespace formaia::domain::entities
