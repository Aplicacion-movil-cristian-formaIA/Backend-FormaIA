#pragma once
#include "core/EventBus.hpp"
#include "db/Database.hpp"
#include "domain/entities/SolicitudIA.hpp"
#include "domain/events/DomainEvents.hpp"
#include "orm/Repository.hpp"

namespace formaia::ia {

// Mantiene la fila de `solicitud_ia` reflejando el estado real del flujo,
// para que GET /api/solicitudes-ia/{id} (polling desde el cliente) pueda
// mostrar progreso sin acoplar ese detalle a cada handler de negocio.
class SolicitudEstadoHandler {
public:
    SolicitudEstadoHandler(core::EventBus& bus, db::ConnectionPool& pool) : pool_(pool) {
        bus.subscribe<domain::events::ReferenteInterpretado>(
            [this](const domain::events::ReferenteInterpretado& e) {
                using namespace domain::entities;
                orm::Repository<SolicitudIA> repo(pool_, SolicitudIA::tabla(), SolicitudIA::columnas());
                auto s = repo.buscarPorId(e.solicitud_id);
                if (!s) return;
                s->referente_detectado = e.hay_referente ? e.referente_nombre : "";
                s->modelo_ia = "groq";
                repo.actualizar(*s, e.solicitud_id);
            });

        bus.subscribe<domain::events::MetaRechazada>(
            [this](const domain::events::MetaRechazada& e) {
                using namespace domain::entities;
                orm::Repository<SolicitudIA> repo(pool_, SolicitudIA::tabla(), SolicitudIA::columnas());
                auto s = repo.buscarPorId(e.solicitud_id);
                if (!s) return;
                s->estado = "rechazada";
                s->motivo_rechazo = e.motivo;
                repo.actualizar(*s, e.solicitud_id);
            });
    }

private:
    db::ConnectionPool& pool_;
};

} // namespace formaia::ia
