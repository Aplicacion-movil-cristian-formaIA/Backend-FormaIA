#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "core/EventBus.hpp"
#include "domain/events/DomainEvents.hpp"
#include "utils/Logger.hpp"

namespace formaia::rutinas {

// Se suscribe a MetaValidada. Arma una rutina en 3 fases (Adaptación,
// Desarrollo, Consolidación) con progresión de duración e intensidad
// (RF-11: "la duración, el volumen y la intensidad aumentarán de forma
// gradual") y semanas de descarga cada 4 semanas (RNF/HU-06). Es lógica
// 100% determinística en C++; la IA ya no interviene en esta parte
// (principio del documento: "la IA propone y las reglas validan").
class RutinaBuilderHandler {
public:
    explicit RutinaBuilderHandler(core::EventBus& bus) : bus_(bus) {
        bus_.subscribe<domain::events::MetaValidada>(
            [this](const domain::events::MetaValidada& e) { manejar(e); });
    }

private:
    void manejar(const domain::events::MetaValidada& evento) {
        using namespace domain::events;

        int semanasTotales = std::max(4, evento.plazo_semanas);
        int dias = std::clamp(evento.dias_disponibles, 1, 6);

        RutinaGenerada rutina;
        rutina.solicitud_id = evento.solicitud_id;
        rutina.usuario_id = evento.usuario_id;
        rutina.correlacion_id = evento.correlacion_id;
        rutina.nombre = "Plan " + evento.arquetipo_nombre;
        rutina.semanas_totales = semanasTotales;

        // Reparto de fases: ~35% adaptación, ~45% desarrollo, resto consolidación.
        int semAdaptacion = std::max(2, (int)std::round(semanasTotales * 0.35));
        int semDesarrollo = std::max(2, (int)std::round(semanasTotales * 0.45));
        int semConsolidacion = std::max(1, semanasTotales - semAdaptacion - semDesarrollo);

        rutina.fases.push_back(construirFase(
            1, "Adaptación", 1, semAdaptacion,
            "Acostumbrar al cuerpo al movimiento y la técnica.",
            /*duracionBase*/ 25, /*duracionInc*/ 1, dias));

        int inicioDesarrollo = semAdaptacion + 1;
        rutina.fases.push_back(construirFase(
            2, "Desarrollo", inicioDesarrollo, inicioDesarrollo + semDesarrollo - 1,
            "Aumentar volumen e intensidad de forma progresiva.",
            /*duracionBase*/ 25 + semAdaptacion, /*duracionInc*/ 2, dias));

        int inicioConsolidacion = inicioDesarrollo + semDesarrollo;
        rutina.fases.push_back(construirFase(
            3, "Consolidación", inicioConsolidacion, semanasTotales,
            "Estabilizar la carga alcanzada y afinar la técnica.",
            /*duracionBase*/ 25 + semAdaptacion + semDesarrollo * 2, /*duracionInc*/ 1, dias));

        utils::Logger::info("Rutina generada para usuario " + evento.usuario_id +
                             ": " + std::to_string(rutina.fases.size()) + " fases, " +
                             std::to_string(semanasTotales) + " semanas");

        bus_.publish(rutina);
    }

    // Arma una fase con una sesión por día disponible, en cada semana del
    // rango, incrementando duración gradualmente y marcando descarga cada
    // 4 semanas (una de cada 4 semanas baja el volumen ~40%).
    domain::events::FaseDTO construirFase(int orden, const std::string& nombre,
                                           int semanaInicio, int semanaFin,
                                           const std::string& objetivo,
                                           int duracionBase, int duracionIncPorSemana,
                                           int diasPorSemana) {
        domain::events::FaseDTO fase;
        fase.orden = orden;
        fase.nombre = nombre;
        fase.semana_inicio = semanaInicio;
        fase.semana_fin = semanaFin;
        fase.objetivo = objetivo;

        for (int semana = semanaInicio; semana <= semanaFin; ++semana) {
            bool esDescarga = (semana % 4 == 0);
            int duracion = duracionBase + duracionIncPorSemana * (semana - semanaInicio);
            if (esDescarga) duracion = std::max(15, (int)(duracion * 0.6));

            for (int d = 1; d <= diasPorSemana; ++d) {
                domain::events::SesionPlanDTO sesion;
                sesion.semana = semana;
                // Reparte los días de entrenamiento a lo largo de la semana
                // (lunes, miércoles, viernes... según cuántos días pidió el usuario).
                sesion.dia_semana = repartirDia(d, diasPorSemana);
                sesion.duracion_min = duracion;
                sesion.es_descarga = esDescarga;
                // Los ids de ejercicio concretos los asigna PersistenceHandler
                // consultando el catálogo por grupo muscular / equipamiento;
                // aquí solo queda el "hueco" con cuántos ejercicios va a tener.
                sesion.ejercicio_ids = {}; // se completa en PersistenceHandler
                fase.sesiones.push_back(sesion);
            }
        }
        return fase;
    }

    static int repartirDia(int indice, int totalDias) {
        // Distribución simple y pareja de 1 a totalDias sobre 7 días.
        static const int patrones[7][6] = {
            {1},
            {1, 4},
            {1, 3, 5},
            {1, 2, 4, 5},
            {1, 2, 3, 5, 6},
            {1, 2, 3, 4, 5, 6},
        };
        int fila = std::clamp(totalDias, 1, 6) - 1;
        return patrones[fila][std::clamp(indice - 1, 0, totalDias - 1)];
    }

    core::EventBus& bus_;
};

} // namespace formaia::rutinas
