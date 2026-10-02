#pragma once
#include <string>
#include <optional>
#include <vector>
#include "orm/Column.hpp"

namespace formaia::domain::entities {

struct Entrenamiento {
    std::string id;
    std::string usuario_id;
    std::string sesion_plan_id;
    std::string iniciado_en;
    std::optional<std::string> finalizado_en;
    bool completado;
    std::optional<int> rpe;

    static std::string tabla() { return "sesion_ejecucion"; }

    static std::vector<orm::Column<Entrenamiento>> columnas() {
        using C = orm::Column<Entrenamiento>;
        return {
            C{"id", true,
              [](const Entrenamiento& e) { return mysqlx::Value(e.id); },
              [](Entrenamiento& e, const mysqlx::Value& v) { e.id = formaia::orm::safe_get_string(v); }},
            C{"usuario_id", false,
              [](const Entrenamiento& e) { return mysqlx::Value(e.usuario_id); },
              [](Entrenamiento& e, const mysqlx::Value& v) { e.usuario_id = formaia::orm::safe_get_string(v); }},
            C{"sesion_plan_id", false,
              [](const Entrenamiento& e) { return mysqlx::Value(e.sesion_plan_id); },
              [](Entrenamiento& e, const mysqlx::Value& v) { e.sesion_plan_id = formaia::orm::safe_get_string(v); }},
            C{"iniciado_en", false,
              [](const Entrenamiento& e) { return mysqlx::Value(e.iniciado_en); },
              [](Entrenamiento& e, const mysqlx::Value& v) { e.iniciado_en = formaia::orm::safe_get_string(v); }},
            C{"finalizado_en", false,
              [](const Entrenamiento& e) { return e.finalizado_en ? mysqlx::Value(*e.finalizado_en) : mysqlx::Value(); },
              [](Entrenamiento& e, const mysqlx::Value& v) { 
                  std::string val = formaia::orm::safe_get_string(v); 
                  if (!val.empty()) e.finalizado_en = val; else e.finalizado_en = std::nullopt;
              }},
            C{"completado", false,
              [](const Entrenamiento& e) { return mysqlx::Value(e.completado ? 1 : 0); },
              [](Entrenamiento& e, const mysqlx::Value& v) { e.completado = !v.isNull() && v.get<int>() != 0; }},
            C{"rpe", false,
              [](const Entrenamiento& e) { return e.rpe ? mysqlx::Value(*e.rpe) : mysqlx::Value(); },
              [](Entrenamiento& e, const mysqlx::Value& v) { 
                  if (!v.isNull()) e.rpe = v.get<int>(); else e.rpe = std::nullopt;
              }}
        };
    }
};

} // namespace formaia::domain::entities
