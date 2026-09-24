#pragma once
#include "core/EventBus.hpp"
#include "domain/events/DomainEvents.hpp"
#include "utils/Logger.hpp"

namespace formaia::notificaciones {

// Último eslabón de la cadena de eventos. Hoy solo registra en el log,
// pero es el punto natural para conectar notificaciones push (FCM/APNs,
// RF-23) o para empujar una actualización por WebSocket al cliente que
// está esperando el resultado de su solicitud, sin acoplar ese código a
// PersistenceHandler ni a MetaSeguridadHandler.
class NotificationHandler {
public:
    explicit NotificationHandler(core::EventBus& bus) {
        bus.subscribe<domain::events::RutinaPersistida>(
            [](const domain::events::RutinaPersistida& e) {
                utils::Logger::info("Notificar a usuario " + e.usuario_id +
                                     ": tu rutina " + e.rutina_id + " ya está lista.");
                // TODO: enviar push / actualizar WebSocket suscrito a e.correlacion_id
            });

        bus.subscribe<domain::events::MetaRechazada>(
            [](const domain::events::MetaRechazada& e) {
                utils::Logger::info("Notificar a usuario " + e.usuario_id +
                                     ": meta rechazada -> " + e.motivo);
                // TODO: enviar push / actualizar WebSocket con la alternativa segura
            });
    }
};

} // namespace formaia::notificaciones
