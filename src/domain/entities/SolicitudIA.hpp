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
    std::string meta_extraida_json;   // JSON como texto
    std::string estado = "aclaracion"; // aclaracion|rechazada|generada|error
    std::string motivo_rechazo;
    std::string modelo_ia;

    static std::string tabla() { return "solicitud_ia"; }

    static std::vector<orm::Column<SolicitudIA>> columnas() {
        using C = orm::Column<SolicitudIA>;
        return {
            C{"id", true,
              [](const SolicitudIA& s) { return mysqlx::Value(s.id); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.id = v.get<std::string>(); }},
            C{"usuario_id", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.usuario_id); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.usuario_id = v.get<std::string>(); }},
            C{"texto_usuario", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.texto_usuario_cifrado_b64); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.texto_usuario_cifrado_b64 = v.get<std::string>(); }},
            C{"referente_detectado", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.referente_detectado); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.referente_detectado = v.get<std::string>(); }},
            C{"referente_ficha_id", false,
              [](const SolicitudIA& s) { return s.referente_ficha_id.empty() ? mysqlx::Value(mysqlx::nullvalue) : mysqlx::Value(s.referente_ficha_id); },
              [](SolicitudIA& s, const mysqlx::Value& v) { if (!v.isNull()) s.referente_ficha_id = v.get<std::string>(); }},
            C{"arquetipo_id", false,
              [](const SolicitudIA& s) { return s.arquetipo_id.empty() ? mysqlx::Value(mysqlx::nullvalue) : mysqlx::Value(s.arquetipo_id); },
              [](SolicitudIA& s, const mysqlx::Value& v) { if (!v.isNull()) s.arquetipo_id = v.get<std::string>(); }},
            C{"meta_extraida", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.meta_extraida_json); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.meta_extraida_json = v.get<std::string>(); }},
            C{"estado", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.estado); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.estado = v.get<std::string>(); }},
            C{"motivo_rechazo", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.motivo_rechazo); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.motivo_rechazo = v.get<std::string>(); }},
            C{"modelo_ia", false,
              [](const SolicitudIA& s) { return mysqlx::Value(s.modelo_ia); },
              [](SolicitudIA& s, const mysqlx::Value& v) { s.modelo_ia = v.get<std::string>(); }},
        };
    }
};

} // namespace formaia::domain::entities
