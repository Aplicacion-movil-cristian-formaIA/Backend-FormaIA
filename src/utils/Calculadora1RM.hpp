#pragma once

namespace formaia::utils {

class Calculadora1RM {
public:
    static double calcularBrzycki(double peso, int reps) {
        if (reps <= 0 || peso <= 0) return 0.0;
        if (reps == 1) return peso;
        // Fórmula de Brzycki: 1RM = Peso * (36 / (37 - Reps))
        // Alternativa simplificada usada: Peso * (1 + 0.0333 * Reps)
        return peso * (1.0 + 0.0333 * reps);
    }
    
    static double calcularEpley(double peso, int reps) {
        if (reps <= 0 || peso <= 0) return 0.0;
        if (reps == 1) return peso;
        // Fórmula de Epley
        return peso * (1.0 + reps / 30.0);
    }
};

} // namespace formaia::utils
