#pragma once
#include <string>
#include <optional>
#include <vector>
#include "orm/Column.hpp"

namespace formaia::domain::entities {

struct RegistroSerie {
    std::string id;
    std::string entrenamiento_id;
    std::string ejercicio_id;
    int numero_serie;
    std::optional<int> repeticiones_hechas;
    std::optional<double> carga_kg;

    static std::string tabla() { return "registro_set"; }

    static std::vector<orm::Column<RegistroSerie>> columnas() {
        using C = orm::Column<RegistroSerie>;
        return {
            C{"id", true,
              [](const RegistroSerie& s) { return mysqlx::Value(s.id); },
              [](RegistroSerie& s, const mysqlx::Value& v) { s.id = formaia::orm::safe_get_string(v); }},
            C{"sesion_ejecucion_id", false,
              [](const RegistroSerie& s) { return mysqlx::Value(s.entrenamiento_id); },
              [](RegistroSerie& s, const mysqlx::Value& v) { s.entrenamiento_id = formaia::orm::safe_get_string(v); }},
            C{"ejercicio_id", false,
              [](const RegistroSerie& s) { return mysqlx::Value(s.ejercicio_id); },
              [](RegistroSerie& s, const mysqlx::Value& v) { s.ejercicio_id = formaia::orm::safe_get_string(v); }},
            C{"numero_serie", false,
              [](const RegistroSerie& s) { return mysqlx::Value(s.numero_serie); },
              [](RegistroSerie& s, const mysqlx::Value& v) { s.numero_serie = v.isNull() ? 1 : v.get<int>(); }},
            C{"repeticiones_hechas", false,
              [](const RegistroSerie& s) { return s.repeticiones_hechas ? mysqlx::Value(*s.repeticiones_hechas) : mysqlx::Value(); },
              [](RegistroSerie& s, const mysqlx::Value& v) { 
                  if (!v.isNull()) s.repeticiones_hechas = v.get<int>(); else s.repeticiones_hechas = std::nullopt;
              }},
            C{"carga_kg", false,
              [](const RegistroSerie& s) { return s.carga_kg ? mysqlx::Value(*s.carga_kg) : mysqlx::Value(); },
              [](RegistroSerie& s, const mysqlx::Value& v) { 
                  if (!v.isNull()) s.carga_kg = v.get<double>(); else s.carga_kg = std::nullopt;
              }}
        };
    }
};

} // namespace formaia::domain::entities
