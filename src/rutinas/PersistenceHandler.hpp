#pragma once
#include <algorithm>
#include <random>

#include "core/EventBus.hpp"
#include "db/Database.hpp"
#include "domain/entities/Ejercicio.hpp"
#include "domain/entities/Rutina.hpp"
#include "domain/entities/SesionEjercicio.hpp"
#include "domain/entities/SolicitudIA.hpp"
#include "domain/events/DomainEvents.hpp"
#include "orm/Repository.hpp"
#include "utils/Logger.hpp"
#include "utils/Uuid.hpp"

namespace formaia::rutinas {

// Se suscribe a RutinaGenerada. Usa Repository<T> (el ORM) para insertar
// rutina + fase + sesion_plan + sesion_ejercicio, eligiendo ejercicios
// reales del catálogo por grupo muscular y equipamiento. Al terminar,
// marca la solicitud_ia como "generada" y publica RutinaPersistida.
class PersistenceHandler {
public:
    explicit PersistenceHandler(core::EventBus& bus, db::ConnectionPool& pool)
        : bus_(bus), pool_(pool) {
        bus_.subscribe<domain::events::RutinaGenerada>(
            [this](const domain::events::RutinaGenerada& e) { manejar(e); });
    }

private:
    void manejar(const domain::events::RutinaGenerada& evento) {
        using namespace formaia::domain::entities;

        orm::Repository<Rutina> repoRutina(pool_, Rutina::tabla(), Rutina::columnas());
        orm::Repository<Fase> repoFase(pool_, Fase::tabla(), Fase::columnas());
        orm::Repository<SesionPlan> repoSesion(pool_, SesionPlan::tabla(), SesionPlan::columnas());
        orm::Repository<SesionEjercicio> repoSesEj(pool_, SesionEjercicio::tabla(), SesionEjercicio::columnas());
        orm::Repository<Ejercicio> repoEjercicio(pool_, Ejercicio::tabla(), Ejercicio::columnas());
        orm::Repository<SolicitudIA> repoSolicitud(pool_, SolicitudIA::tabla(), SolicitudIA::columnas());

        // Desactivar rutinas anteriores para evitar uq_rutina_activa_por_usuario
        try {
            auto conn = pool_.acquire();
            conn->sql("UPDATE rutina SET activa = false WHERE usuario_id = ?")
                .bind(evento.usuario_id)
                .execute();
        } catch (const std::exception& e) {
            utils::Logger::error(std::string("Error desactivando rutinas: ") + e.what());
        }

        Rutina rutina;
        rutina.id = utils::newUuid();
        rutina.usuario_id = evento.usuario_id;
        rutina.solicitud_id = evento.solicitud_id;
        rutina.nombre = evento.nombre;
        rutina.semanas_totales = evento.semanas_totales;
        rutina.activa = true;
        repoRutina.insertar(rutina);

        // Catálogo de ejercicios por grupo muscular, elegido una sola vez
        // y reutilizado para todas las sesiones (evita golpear la BD por
        // cada sesión individual).
        auto pecho = repoEjercicio.buscarTodosPor("grupo_muscular = :valor", "pecho");
        auto espalda = repoEjercicio.buscarTodosPor("grupo_muscular = :valor", "espalda");
        auto piernas = repoEjercicio.buscarTodosPor("grupo_muscular = :valor", "piernas");
        auto core_ = repoEjercicio.buscarTodosPor("grupo_muscular = :valor", "core");
        std::vector<Ejercicio> catalogo;
        for (auto* grupo : {&pecho, &espalda, &piernas, &core_})
            catalogo.insert(catalogo.end(), grupo->begin(), grupo->end());

        for (auto& faseDto : evento.fases) {
            Fase fase;
            fase.id = utils::newUuid();
            fase.rutina_id = rutina.id;
            fase.orden = faseDto.orden;
            fase.nombre = faseDto.nombre;
            fase.semana_inicio = faseDto.semana_inicio;
            fase.semana_fin = faseDto.semana_fin;
            fase.objetivo = faseDto.objetivo;
            repoFase.insertar(fase);

            for (auto& sesionDto : faseDto.sesiones) {
                SesionPlan sesion;
                sesion.id = utils::newUuid();
                sesion.fase_id = fase.id;
                sesion.semana = sesionDto.semana;
                sesion.dia_semana = sesionDto.dia_semana;
                sesion.duracion_min = sesionDto.duracion_min;
                sesion.es_descarga = sesionDto.es_descarga;
                repoSesion.insertar(sesion);

                asignarEjercicios(repoSesEj, sesion, catalogo, sesionDto.es_descarga);
            }
        }

        // Marca la solicitud como generada.
        auto solicitudOpt = repoSolicitud.buscarPorId(evento.solicitud_id);
        if (solicitudOpt) {
            solicitudOpt->estado = "generada";
            repoSolicitud.actualizar(*solicitudOpt, evento.solicitud_id);
        }

        utils::Logger::info("Rutina " + rutina.id + " persistida para usuario " + evento.usuario_id);

        domain::events::RutinaPersistida out;
        out.rutina_id = rutina.id;
        out.usuario_id = evento.usuario_id;
        out.solicitud_id = evento.solicitud_id;
        out.correlacion_id = evento.correlacion_id;
        bus_.publish(out);
    }

    void asignarEjercicios(orm::Repository<domain::entities::SesionEjercicio>& repoSesEj,
                            const domain::entities::SesionPlan& sesion,
                            const std::vector<domain::entities::Ejercicio>& catalogo,
                            bool esDescarga) {
        if (catalogo.empty()) return;

        static thread_local std::mt19937 rng{std::random_device{}()};
        std::vector<domain::entities::Ejercicio> elegidos = catalogo;
        std::shuffle(elegidos.begin(), elegidos.end(), rng);

        int cantidad = esDescarga ? 3 : std::min<int>(6, (int)elegidos.size());
        for (int i = 0; i < cantidad && i < (int)elegidos.size(); ++i) {
            domain::entities::SesionEjercicio se;
            se.id = utils::newUuid();
            se.sesion_plan_id = sesion.id;
            se.ejercicio_id = elegidos[i].id;
            se.orden = i + 1;
            se.series = esDescarga ? 2 : 3;
            se.repeticiones = "10-12";
            se.descanso_seg = 60;
            repoSesEj.insertar(se);
        }
    }

    core::EventBus& bus_;
    db::ConnectionPool& pool_;
};

} // namespace formaia::rutinas
