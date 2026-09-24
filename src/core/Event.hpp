#pragma once
#include <any>
#include <string>
#include <typeindex>

namespace formaia::core {

// Un Event de dominio es simplemente: "algo con este tipo de nombre pasó,
// y aquí va su payload". El EventBus despacha por std::type_index del
// payload real (ver EventBus.hpp), así que Event en sí solo sirve como
// envoltorio de trazabilidad (id de correlación + nombre legible) que
// los handlers pueden usar para logging/auditoría.
struct EventMeta {
    std::string nombre;           // Ej. "SolicitudIACreada"
    std::string correlacion_id;   // Para rastrear un flujo completo en logs
};

} // namespace formaia::core
