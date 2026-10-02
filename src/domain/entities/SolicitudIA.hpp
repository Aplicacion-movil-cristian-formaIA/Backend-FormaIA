#pragma once
#include <string>
#include <vector>
#include "orm/Column.hpp"

namespace formaia::domain::entities {

struct SolicitudIA {
    std::string id;
    std::string usuario_id;
    std::string texto_usuario_cifrado_b64;
    std::string referente_detectado;
    std::string referente_ficha_id;   // puede quedar vacío
    std::string arquetipo_id;         // puede quedar vacío
    std::string meta_extraida_json = "{}";   // JSON como texto
    std::string estado = "aclaracion"; // aclaracion|rechazada|generada|error
    std::string motivo_rechazo;
    std::string modelo_ia;

    static std::string tabla() { return "solicitud_ia"; }

    static std::vector<orm::Column<SolicitudIA>> columnas() {
        using C = orm::Column<SolicitudIA>;
        
        return {
            C{"id", true,
              [](const SolicitudIA& s) { return mysqlx::Value(s.id); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.id = formaia::orm::safe_get_string(v); }},
            C{"usuario_id", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.usuario_id); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.usuario_id = formaia::orm::safe_get_string(v); }},
            C{"texto_usuario", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.texto_usuario_cifrado_b64); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.texto_usuario_cifrado_b64 = formaia::orm::safe_get_string(v); }},
            C{"referente_detectado", false,
              [](const SolicitudIA& s) { return s.referente_detectado.empty() ? mysqlx::Value(mysqlx::nullvalue) : mysqlx::Value(s.referente_detectado); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.referente_detectado = formaia::orm::safe_get_string(v); }},
            C{"referente_ficha_id", false,
              [](const SolicitudIA& s) { return s.referente_ficha_id.empty() ? mysqlx::Value(mysqlx::nullvalue) : mysqlx::Value(s.referente_ficha_id); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.referente_ficha_id = formaia::orm::safe_get_string(v); }},
            C{"arquetipo_id", false,
              [](const SolicitudIA& s) { return s.arquetipo_id.empty() ? mysqlx::Value(mysqlx::nullvalue) : mysqlx::Value(s.arquetipo_id); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.arquetipo_id = formaia::orm::safe_get_string(v); }},
            C{"meta_extraida", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.meta_extraida_json); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.meta_extraida_json = formaia::orm::safe_get_string(v); if(s.meta_extraida_json.empty()) s.meta_extraida_json = "{}"; }},
            C{"estado", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.estado); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.estado = formaia::orm::safe_get_string(v); }},
            C{"motivo_rechazo", false,
              [](const SolicitudIA& s) { return s.motivo_rechazo.empty() ? mysqlx::Value(mysqlx::nullvalue) : mysqlx::Value(s.motivo_rechazo); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.motivo_rechazo = formaia::orm::safe_get_string(v); }},
            C{"modelo_ia", false,
              [](const SolicitudIA& s) { return s.modelo_ia.empty() ? mysqlx::Value(mysqlx::nullvalue) : mysqlx::Value(s.modelo_ia); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.modelo_ia = formaia::orm::safe_get_string(v); }},
        };
    }
};

} // namespace formaia::domain::entities
