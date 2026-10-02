#pragma once
#include "core/EventBus.hpp"
#include "db/Database.hpp"
#include "domain/events/DomainEvents.hpp"
#include "utils/Logger.hpp"

namespace formaia::rutinas {

class ProgressionHandler {
public:
    ProgressionHandler(core::EventBus& bus, db::ConnectionPool& pool)
        : bus_(bus), pool_(pool) {
        bus_.subscribe<domain::events::SesionCompletada>(
            [this](const domain::events::SesionCompletada& e) { manejar(e); });
    }

private:
    void manejar(const domain::events::SesionCompletada& e) {
        // Implementación de Sobrecarga Progresiva / Autorregulación
        // Fase 1 de la investigación: Feedback Loop (RPE)
        
        utils::Logger::info("ProgressionHandler: Procesando RPE " + std::to_string(e.rpe) + 
                            " para sesion " + e.sesion_id);

        try {
            auto conn = pool_.acquire();
            
            // 1. Obtener a qué fase y semana pertenece la sesión actual
            auto result = conn->sql(
                "SELECT f.rutina_id, s.semana "
                "FROM sesion_plan s "
                "JOIN fase f ON s.fase_id = f.id "
                "WHERE s.id = ?"
            ).bind(e.sesion_id).execute();

            auto row = result.fetchOne();
            if (!row) {
                utils::Logger::error("No se encontro la sesion " + e.sesion_id);
                return;
            }

            std::string rutina_id = static_cast<std::string>(row[0]);
            int semana_actual = static_cast<int>(row[1]);

            // 2. Lógica de ajuste basada en RPE
            // Escala RPE (1-10): 
            // 1-4: Muy fácil (Aumentar carga)
            // 5-7: Moderado (Mantener progresión estándar)
            // 8-10: Muy duro (Reducir carga para evitar sobreentrenamiento)
            
            int ajuste_minutos = 0;
            if (e.rpe <= 4) {
                ajuste_minutos = 5; // Aumentar volumen
                utils::Logger::info("RPE bajo, aumentando intensidad futura.");
            } else if (e.rpe >= 8) {
                ajuste_minutos = -5; // Reducir volumen
                utils::Logger::info("RPE muy alto, reduciendo intensidad futura.");
            } else {
                utils::Logger::info("RPE óptimo, se mantiene la progresión base.");
                return; // No hay cambios
            }

            // 3. Aplicar el ajuste a TODAS las sesiones FUTURAS de esta rutina
            conn->sql(
                "UPDATE sesion_plan s "
                "JOIN fase f ON s.fase_id = f.id "
                "SET s.duracion_min = GREATEST(10, s.duracion_min + ?) "
                "WHERE f.rutina_id = ? AND s.semana > ?"
            ).bind(ajuste_minutos, rutina_id, semana_actual).execute();

        } catch (const std::exception& ex) {
            utils::Logger::error("Error en ProgressionHandler: " + std::string(ex.what()));
        }
    }

    core::EventBus& bus_;
    db::ConnectionPool& pool_;
};

} // namespace formaia::rutinas
