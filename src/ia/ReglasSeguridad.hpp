#pragma once
#include <algorithm>
#include <cmath>
#include <string>

#include "domain/events/DomainEvents.hpp"

namespace formaia::ia {

// ---------------------------------------------------------------------
// ReglasSeguridad implementa RF-09 ("el sistema validará la meta...") y
// RNF-07 ("filtros de entrada y salida... sin metas para menores fuera
// de rango saludable"). Es código C++ puro, determinístico: la IA NUNCA
// decide si una meta es segura, solo interpreta el lenguaje natural.
// Esto es intencional -un modelo de lenguaje puede "alucinar" un límite
// saludable, el código no.
// ---------------------------------------------------------------------
struct ResultadoValidacion {
    bool aprobada = false;
    std::string motivo;
    double peso_objetivo_sugerido_kg = 0;
    int plazo_sugerido_semanas = 0;
};

class ReglasSeguridad {
public:
    // Ritmo saludable de pérdida de peso ampliamente aceptado: 0.5 a 1 kg
    // por semana. Se usa el límite superior para no ser demasiado
    // restrictivo, pero nunca se permite superarlo.
    static constexpr double kMaxPerdidaKgPorSemana = 1.0;
    static constexpr double kMinPerdidaKgPorSemana = 0.3;

    // Límite de ganancia de masa muscular "limpia" (sin considerarse
    // ganancia de grasa) ampliamente citado en literatura de fuerza.
    static constexpr double kMaxGananciaKgPorSemana = 0.5;

    static constexpr double kImcMinimoSaludable = 18.5;
    static constexpr double kImcMaximoSaludable = 27.0;

    static ResultadoValidacion validar(const domain::events::ReferenteInterpretado& interpretacion,
                                        double peso_actual_kg,
                                        double estatura_cm,
                                        bool es_menor_de_edad) {
        ResultadoValidacion r;

        if (peso_actual_kg <= 0 || estatura_cm <= 0) {
            r.motivo = "Faltan datos de peso o estatura del usuario en su perfil.";
            return r;
        }

        double estatura_m = estatura_cm / 100.0;
        double imc_actual = peso_actual_kg / (estatura_m * estatura_m);

        // 1) Menores de edad: nunca se permite una meta de restricción
        //    calórica o cambio corporal agresivo (RF-04 / RNF-07).
        if (es_menor_de_edad && interpretacion.tipo_meta == "bajar_peso") {
            r.motivo = "Para menores de edad no se generan metas de pérdida de peso; "
                       "se ofrece un plan de acondicionamiento general.";
            return r;
        }

        // 2) Calcular una meta segura según el tipo de meta y el plazo pedido.
        double semanas = std::max(4, interpretacion.plazo_semanas);
        double delta_maximo = 0;

        if (interpretacion.tipo_meta == "bajar_peso") {
            delta_maximo = kMaxPerdidaKgPorSemana * semanas;
            double peso_objetivo = peso_actual_kg - delta_maximo;
            double imc_objetivo = peso_objetivo / (estatura_m * estatura_m);

            // No se permite bajar del IMC mínimo saludable.
            if (imc_objetivo < kImcMinimoSaludable) {
                peso_objetivo = kImcMinimoSaludable * estatura_m * estatura_m;
            }
            r.peso_objetivo_sugerido_kg = std::round(peso_objetivo * 10) / 10.0;
        } else if (interpretacion.tipo_meta == "ganar_masa") {
            delta_maximo = kMaxGananciaKgPorSemana * semanas;
            r.peso_objetivo_sugerido_kg = std::round((peso_actual_kg + delta_maximo) * 10) / 10.0;
        } else {
            r.peso_objetivo_sugerido_kg = peso_actual_kg; // definir/mantener: sin cambio de peso objetivo
        }

        // 3) ¿La meta IMPLÍCITA del usuario (si mencionó un plazo o un
        //    número en su mensaje que el backend haya podido inferir)
        //    excede el ritmo máximo? Esa comparación específica ocurre
        //    en el handler que llama a esta función, comparando contra
        //    lo que el usuario pidió explícitamente (por eso aquí solo
        //    calculamos el límite; el handler decide aprobar/rechazar
        //    comparando contra lo pedido).
        r.plazo_sugerido_semanas = static_cast<int>(semanas);
        r.aprobada = true;
        return r;
    }

    // Compara lo que el usuario pidió explícitamente contra el límite
    // saludable calculado arriba. Se usa cuando el texto del usuario
    // incluye una cifra concreta (ej. "bajar 15 kg en 2 semanas").
    static ResultadoValidacion validarPeticionExplicita(double peso_actual_kg,
                                                          double kg_pedidos,
                                                          int semanas_pedidas,
                                                          const std::string& tipo_meta) {
        ResultadoValidacion r;
        if (semanas_pedidas <= 0) semanas_pedidas = 1;
        double ritmo_pedido = kg_pedidos / static_cast<double>(semanas_pedidas);

        double limite = (tipo_meta == "ganar_masa") ? kMaxGananciaKgPorSemana : kMaxPerdidaKgPorSemana;

        if (ritmo_pedido > limite) {
            r.aprobada = false;
            r.motivo = "El ritmo solicitado (" + std::to_string(ritmo_pedido) +
                       " kg/semana) supera el máximo saludable recomendado (" +
                       std::to_string(limite) + " kg/semana).";
            // Alternativa segura: mismo objetivo de kg, pero al ritmo máximo permitido.
            r.plazo_sugerido_semanas = static_cast<int>(std::ceil(kg_pedidos / limite));
            r.peso_objetivo_sugerido_kg = (tipo_meta == "ganar_masa")
                ? peso_actual_kg + kg_pedidos
                : peso_actual_kg - kg_pedidos;
            return r;
        }

        r.aprobada = true;
        return r;
    }
};

} // namespace formaia::ia
