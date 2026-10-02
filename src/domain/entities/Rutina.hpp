#pragma once
#include <string>
#include <vector>
#include "orm/Column.hpp"

namespace formaia::domain::entities {

struct Rutina {
    std::string id;
    std::string usuario_id;
    std::string solicitud_id;
    std::string nombre;
    int semanas_totales = 12;
    bool activa = true;
    int version = 1;

    static std::string tabla() { return "rutina"; }

    static std::vector<orm::Column<Rutina>> columnas() {
        using C = orm::Column<Rutina>;
        return {
            C{"id", true,
              [](const Rutina& r) { return mysqlx::Value(r.id); },
              [](Rutina& r, const mysqlx::Value& v) { r.id = formaia::orm::safe_get_string(v); }},
            C{"usuario_id", false,
              [](const Rutina& r) { return mysqlx::Value(r.usuario_id); },
              [](Rutina& r, const mysqlx::Value& v) { r.usuario_id = formaia::orm::safe_get_string(v); }},
            C{"solicitud_id", false,
              [](const Rutina& r) { return mysqlx::Value(r.solicitud_id); },
              [](Rutina& r, const mysqlx::Value& v) { r.solicitud_id = formaia::orm::safe_get_string(v); }},
            C{"nombre", false,
              [](const Rutina& r) { return mysqlx::Value(r.nombre); },
              [](Rutina& r, const mysqlx::Value& v) { r.nombre = formaia::orm::safe_get_string(v); }},
            C{"semanas_totales", false,
              [](const Rutina& r) { return mysqlx::Value(r.semanas_totales); },
              [](Rutina& r, const mysqlx::Value& v) { r.semanas_totales = (int)v.get<int64_t>(); }},
            C{"activa", false,
              [](const Rutina& r) { return mysqlx::Value(r.activa ? 1 : 0); },
              [](Rutina& r, const mysqlx::Value& v) { r.activa = v.get<int64_t>() != 0; }},
            C{"version", false,
              [](const Rutina& r) { return mysqlx::Value(r.version); },
              [](Rutina& r, const mysqlx::Value& v) { r.version = (int)v.get<int64_t>(); }},
        };
    }
};

struct Fase {
    std::string id;
    std::string rutina_id;
    int orden = 1;
    std::string nombre;
    int semana_inicio = 1;
    int semana_fin = 4;
    std::string objetivo;

    static std::string tabla() { return "fase"; }

    static std::vector<orm::Column<Fase>> columnas() {
        using C = orm::Column<Fase>;
        return {
            C{"id", true,
              [](const Fase& f) { return mysqlx::Value(f.id); },
              [](Fase& f, const mysqlx::Value& v) { f.id = formaia::orm::safe_get_string(v); }},
            C{"rutina_id", false,
              [](const Fase& f) { return mysqlx::Value(f.rutina_id); },
              [](Fase& f, const mysqlx::Value& v) { f.rutina_id = formaia::orm::safe_get_string(v); }},
            C{"orden", false,
              [](const Fase& f) { return mysqlx::Value(f.orden); },
              [](Fase& f, const mysqlx::Value& v) { f.orden = (int)v.get<int64_t>(); }},
            C{"nombre", false,
              [](const Fase& f) { return mysqlx::Value(f.nombre); },
              [](Fase& f, const mysqlx::Value& v) { f.nombre = formaia::orm::safe_get_string(v); }},
            C{"semana_inicio", false,
              [](const Fase& f) { return mysqlx::Value(f.semana_inicio); },
              [](Fase& f, const mysqlx::Value& v) { f.semana_inicio = (int)v.get<int64_t>(); }},
            C{"semana_fin", false,
              [](const Fase& f) { return mysqlx::Value(f.semana_fin); },
              [](Fase& f, const mysqlx::Value& v) { f.semana_fin = (int)v.get<int64_t>(); }},
            C{"objetivo", false,
              [](const Fase& f) { return mysqlx::Value(f.objetivo); },
              [](Fase& f, const mysqlx::Value& v) { f.objetivo = formaia::orm::safe_get_string(v); }},
        };
    }
};

struct SesionPlan {
    std::string id;
    std::string fase_id;
    int semana = 1;
    int dia_semana = 1;
    int duracion_min = 30;
    bool es_descarga = false;

    static std::string tabla() { return "sesion_plan"; }

    static std::vector<orm::Column<SesionPlan>> columnas() {
        using C = orm::Column<SesionPlan>;
        return {
            C{"id", true,
              [](const SesionPlan& s) { return mysqlx::Value(s.id); },
              [](SesionPlan& s, const mysqlx::Value& v) { s.id = formaia::orm::safe_get_string(v); }},
            C{"fase_id", false,
              [](const SesionPlan& s) { return mysqlx::Value(s.fase_id); },
              [](SesionPlan& s, const mysqlx::Value& v) { s.fase_id = formaia::orm::safe_get_string(v); }},
            C{"semana", false,
              [](const SesionPlan& s) { return mysqlx::Value(s.semana); },
              [](SesionPlan& s, const mysqlx::Value& v) { s.semana = (int)v.get<int64_t>(); }},
            C{"dia_semana", false,
              [](const SesionPlan& s) { return mysqlx::Value(s.dia_semana); },
              [](SesionPlan& s, const mysqlx::Value& v) { s.dia_semana = (int)v.get<int64_t>(); }},
            C{"duracion_min", false,
              [](const SesionPlan& s) { return mysqlx::Value(s.duracion_min); },
              [](SesionPlan& s, const mysqlx::Value& v) { s.duracion_min = (int)v.get<int64_t>(); }},
            C{"es_descarga", false,
              [](const SesionPlan& s) { return mysqlx::Value(s.es_descarga ? 1 : 0); },
              [](SesionPlan& s, const mysqlx::Value& v) { s.es_descarga = v.get<int64_t>() != 0; }},
        };
    }
};

} // namespace formaia::domain::entities
