#pragma once
#include <string>
#include <vector>
#include "orm/Column.hpp"

namespace formaia::domain::entities {

struct SesionEjercicio {
    std::string id;
    std::string sesion_plan_id;
    std::string ejercicio_id;
    int orden = 1;
    int series = 3;
    std::string repeticiones = "10-12";
    int descanso_seg = 60;
    std::string carga_sugerida;

    static std::string tabla() { return "sesion_ejercicio"; }

    static std::vector<orm::Column<SesionEjercicio>> columnas() {
        using C = orm::Column<SesionEjercicio>;
        return {
            C{"id", true,
              [](const SesionEjercicio& s) { return mysqlx::Value(s.id); },
              [](SesionEjercicio& s, const mysqlx::Value& v) { s.id = formaia::orm::safe_get_string(v); }},
            C{"sesion_plan_id", false,
              [](const SesionEjercicio& s) { return mysqlx::Value(s.sesion_plan_id); },
              [](SesionEjercicio& s, const mysqlx::Value& v) { s.sesion_plan_id = formaia::orm::safe_get_string(v); }},
            C{"ejercicio_id", false,
              [](const SesionEjercicio& s) { return mysqlx::Value(s.ejercicio_id); },
              [](SesionEjercicio& s, const mysqlx::Value& v) { s.ejercicio_id = formaia::orm::safe_get_string(v); }},
            C{"orden", false,
              [](const SesionEjercicio& s) { return mysqlx::Value(s.orden); },
              [](SesionEjercicio& s, const mysqlx::Value& v) { s.orden = (int)v.get<int64_t>(); }},
            C{"series", false,
              [](const SesionEjercicio& s) { return mysqlx::Value(s.series); },
              [](SesionEjercicio& s, const mysqlx::Value& v) { s.series = (int)v.get<int64_t>(); }},
            C{"repeticiones", false,
              [](const SesionEjercicio& s) { return mysqlx::Value(s.repeticiones); },
              [](SesionEjercicio& s, const mysqlx::Value& v) { s.repeticiones = formaia::orm::safe_get_string(v); }},
            C{"descanso_seg", false,
              [](const SesionEjercicio& s) { return mysqlx::Value(s.descanso_seg); },
              [](SesionEjercicio& s, const mysqlx::Value& v) { s.descanso_seg = (int)v.get<int64_t>(); }},
            C{"carga_sugerida", false,
              [](const SesionEjercicio& s) { return mysqlx::Value(s.carga_sugerida); },
              [](SesionEjercicio& s, const mysqlx::Value& v) { s.carga_sugerida = formaia::orm::safe_get_string(v); }},
        };
    }
};

} // namespace formaia::domain::entities
