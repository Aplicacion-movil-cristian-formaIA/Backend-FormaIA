#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "utils/Calculadora1RM.hpp"

using namespace formaia::utils;

TEST_CASE("Calculadora1RM calcula el One Rep Max correctamente", "[core][calculadora]") {

    SECTION("Brzycki con 1 repetición debe retornar el mismo peso") {
        double orm = Calculadora1RM::calcularBrzycki(100.0, 1);
        REQUIRE(orm == 100.0);
    }

    SECTION("Brzycki con 10 repeticiones debe retornar un valor mayor") {
        double orm = Calculadora1RM::calcularBrzycki(100.0, 10);
        // 100 * (1 + 0.0333 * 10) = 100 * 1.333 = 133.3
        REQUIRE(orm == Catch::Approx(133.3).epsilon(0.01));
    }

    SECTION("Si reps o peso es <= 0 retorna 0") {
        REQUIRE(Calculadora1RM::calcularBrzycki(0, 10) == 0.0);
        REQUIRE(Calculadora1RM::calcularBrzycki(100, 0) == 0.0);
        REQUIRE(Calculadora1RM::calcularBrzycki(-50, 10) == 0.0);
    }

    SECTION("Epley con 1 repetición debe retornar el mismo peso") {
        double orm = Calculadora1RM::calcularEpley(100.0, 1);
        REQUIRE(orm == 100.0);
    }

    SECTION("Epley con 5 repeticiones") {
        double orm = Calculadora1RM::calcularEpley(100.0, 5);
        // 100 * (1 + 5/30) = 100 * 1.1666... = 116.666...
        REQUIRE(orm == Catch::Approx(116.666).epsilon(0.01));
    }
}
