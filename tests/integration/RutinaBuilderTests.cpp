#include <catch2/catch_test_macros.hpp>
#include "core/EventBus.hpp"
#include "domain/events/DomainEvents.hpp"
#include "rutinas/RutinaBuilderHandler.hpp"
#include <boost/asio.hpp>
#include <memory>

using namespace formaia::core;
using namespace formaia::domain::events;
using namespace formaia::rutinas;

TEST_CASE("RutinaBuilderHandler genera rutina completa", "[integracion][rutinas]") {
    boost::asio::io_context io;
    EventBus bus(io);
    RutinaBuilderHandler handler(bus);

    std::shared_ptr<RutinaGenerada> rutinaGenerada;

    bus.subscribe<RutinaGenerada>([&rutinaGenerada](const RutinaGenerada& r) {
        rutinaGenerada = std::make_shared<RutinaGenerada>(r);
    });

    SECTION("Se crean fases y sesiones progresivas") {
        MetaValidada meta;
        meta.solicitud_id = "req-1";
        meta.usuario_id = "usr-1";
        meta.arquetipo_nombre = "Fuerza";
        meta.dias_disponibles = 3;
        meta.plazo_semanas = 12;

        bus.publish(meta);
        
        // Ejecutar los handlers pendientes (RutinaBuilderHandler y luego RutinaGenerada)
        io.run();

        REQUIRE(rutinaGenerada != nullptr);
        REQUIRE(rutinaGenerada->solicitud_id == "req-1");
        REQUIRE(rutinaGenerada->fases.size() == 3);

        auto& f1 = rutinaGenerada->fases[0];
        REQUIRE(f1.nombre == "Adaptación");
        
        // Cada semana debe tener 3 dias
        int totalSesiones = 0;
        for (auto& fase : rutinaGenerada->fases) {
            totalSesiones += fase.sesiones.size();
        }
        
        // 12 semanas * 3 dias = 36 sesiones
        REQUIRE(totalSesiones == 36);

        // La semana 4 debe ser de descarga
        bool descargoSemana4 = false;
        for (auto& f : rutinaGenerada->fases) {
            for (auto& s : f.sesiones) {
                if (s.semana == 4) descargoSemana4 = s.es_descarga;
            }
        }
        REQUIRE(descargoSemana4 == true);
    }
}
