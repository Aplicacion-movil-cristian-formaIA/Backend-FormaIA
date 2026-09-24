#pragma once
#include <string>
#include <vector>
#include "orm/Column.hpp"

namespace formaia::domain::entities {

struct Ejercicio {
    std::string id;
    std::string nombre;
    std::string grupo_muscular;
    std::string equipamiento = "sin_equipo";
    std::string nivel = "principiante";
    std::string video_url;
    std::string contraindicaciones_json = "[]"; // JSON: ["rodilla", ...]

    static std::string tabla() { return "ejercicio"; }

    static std::vector<orm::Column<Ejercicio>> columnas() {
        using C = orm::Column<Ejercicio>;
        return {
            C{"id", true,
              [](const Ejercicio& e) { return mysqlx::Value(e.id); },
              [](Ejercicio& e, const mysqlx::Value& v) { e.id = v.get<std::string>(); }},
            C{"nombre", false,
              [](const Ejercicio& e) { return mysqlx::Value(e.nombre); },
              [](Ejercicio& e, const mysqlx::Value& v) { e.nombre = v.get<std::string>(); }},
            C{"grupo_muscular", false,
              [](const Ejercicio& e) { return mysqlx::Value(e.grupo_muscular); },
              [](Ejercicio& e, const mysqlx::Value& v) { e.grupo_muscular = v.get<std::string>(); }},
            C{"equipamiento", false,
              [](const Ejercicio& e) { return mysqlx::Value(e.equipamiento); },
              [](Ejercicio& e, const mysqlx::Value& v) { e.equipamiento = v.get<std::string>(); }},
            C{"nivel", false,
              [](const Ejercicio& e) { return mysqlx::Value(e.nivel); },
              [](Ejercicio& e, const mysqlx::Value& v) { e.nivel = v.get<std::string>(); }},
            C{"video_url", false,
              [](const Ejercicio& e) { return mysqlx::Value(e.video_url); },
              [](Ejercicio& e, const mysqlx::Value& v) { e.video_url = v.get<std::string>(); }},
            C{"contraindicaciones", false,
              [](const Ejercicio& e) { return mysqlx::Value(e.contraindicaciones_json); },
              [](Ejercicio& e, const mysqlx::Value& v) { e.contraindicaciones_json = v.get<std::string>(); }},
        };
    }
};

} // namespace formaia::domain::entities
