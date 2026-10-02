#include <catch2/catch_test_macros.hpp>
#include "core/EventBus.hpp"
#include <boost/asio.hpp>
#include <atomic>
#include <thread>

using namespace formaia::core;

struct EventoPrueba {
    int valor;
};

struct OtroEventoPrueba {
    std::string texto;
};

TEST_CASE("EventBus publica y suscribe eventos correctamente", "[eventbus]") {
    boost::asio::io_context io;
    EventBus bus(io);

    SECTION("Un suscriptor recibe el evento") {
        std::atomic<int> recibido{0};

        bus.subscribe<EventoPrueba>([&recibido](const EventoPrueba& e) {
            recibido = e.valor;
        });

        bus.publish(EventoPrueba{42});

        // Corremos io_context.run_one() para procesar la cola
        io.run_one();

        REQUIRE(recibido.load() == 42);
    }

    SECTION("Multiples suscriptores reciben el evento") {
        std::atomic<int> sum{0};

        bus.subscribe<EventoPrueba>([&sum](const EventoPrueba& e) { sum += e.valor; });
        bus.subscribe<EventoPrueba>([&sum](const EventoPrueba& e) { sum += e.valor; });

        bus.publish(EventoPrueba{10});

        // Necesita procesar al menos dos posts
        io.poll();

        REQUIRE(sum.load() == 20);
    }

    SECTION("No recibe eventos no suscritos") {
        std::atomic<bool> llamo{false};

        bus.subscribe<EventoPrueba>([&llamo](const EventoPrueba&) { llamo = true; });
        bus.publish(OtroEventoPrueba{"hola"});

        io.poll();

        REQUIRE(llamo.load() == false);
    }
}
