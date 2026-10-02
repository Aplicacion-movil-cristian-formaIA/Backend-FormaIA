#pragma once
#include <string>
#include <vector>
#include "orm/Column.hpp"

namespace formaia::domain::entities {

struct PerfilFisico {
    std::string usuario_id;       // clave primaria (1:1 con usuario)
    std::string sexo;             // femenino | masculino | prefiero_no_decir
    std::string estatura_cm_cifrada_b64;
    std::string peso_kg_cifrada_b64;
    std::string nivel = "principiante";
    int dias_semana = 3;
    int minutos_sesion = 30;
    std::string equipamiento_json = "[]";

    static std::string tabla() { return "perfil_fisico"; }

    static std::vector<orm::Column<PerfilFisico>> columnas() {
        using C = orm::Column<PerfilFisico>;
        
        return {
            C{"usuario_id", true,
              [](const PerfilFisico& p) { return mysqlx::Value(p.usuario_id); },
              [](PerfilFisico& p, const mysqlx::Value& v) { p.usuario_id = formaia::orm::safe_get_string(v); }},
            C{"sexo", false,
              [](const PerfilFisico& p) { return mysqlx::Value(p.sexo); },
              [](PerfilFisico& p, const mysqlx::Value& v) { p.sexo = formaia::orm::safe_get_string(v); }},
            C{"estatura_cm", false,
              [](const PerfilFisico& p) { return mysqlx::Value(p.estatura_cm_cifrada_b64); },
              [](PerfilFisico& p, const mysqlx::Value& v) { p.estatura_cm_cifrada_b64 = formaia::orm::safe_get_string(v); }},
            C{"peso_kg", false,
              [](const PerfilFisico& p) { return mysqlx::Value(p.peso_kg_cifrada_b64); },
              [](PerfilFisico& p, const mysqlx::Value& v) { p.peso_kg_cifrada_b64 = formaia::orm::safe_get_string(v); }},
            C{"nivel", false,
              [](const PerfilFisico& p) { return mysqlx::Value(p.nivel); },
              [](PerfilFisico& p, const mysqlx::Value& v) { p.nivel = formaia::orm::safe_get_string(v); }},
            C{"dias_semana", false,
              [](const PerfilFisico& p) { return mysqlx::Value(p.dias_semana); },
              [](PerfilFisico& p, const mysqlx::Value& v) { p.dias_semana = (int)v.get<int64_t>(); }},
            C{"minutos_sesion", false,
              [](const PerfilFisico& p) { return mysqlx::Value(p.minutos_sesion); },
              [](PerfilFisico& p, const mysqlx::Value& v) { p.minutos_sesion = (int)v.get<int64_t>(); }},
            C{"equipamiento", false,
              [](const PerfilFisico& p) { return mysqlx::Value(p.equipamiento_json); },
              [](PerfilFisico& p, const mysqlx::Value& v) { p.equipamiento_json = formaia::orm::safe_get_string(v); }},
        };
    }
};

} // namespace formaia::domain::entities
