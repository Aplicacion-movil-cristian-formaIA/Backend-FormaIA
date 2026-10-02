#include <thread>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include "config/Config.hpp"
#include "core/EventBus.hpp"
#include "db/Database.hpp"
#include "http/HttpServer.hpp"
#include "http/Router.hpp"
#include "http/routes/RutinaController.hpp"
#include "http/routes/SolicitudController.hpp"
#include "http/routes/UsuarioController.hpp"
#include "http/routes/EjecucionController.hpp"
#include "http/routes/AlternativasController.hpp"
#include "http/routes/EvolucionController.hpp"
#include "http/routes/SwaggerController.hpp"
#include "ia/GroqClient.hpp"
#include "ia/IAOrchestratorHandler.hpp"
#include "ia/MetaSeguridadHandler.hpp"
#include "ia/SolicitudEstadoHandler.hpp"
#include "notificaciones/NotificationHandler.hpp"
#include "rutinas/PersistenceHandler.hpp"
#include "rutinas/ProgressionHandler.hpp"
#include "rutinas/RutinaBuilderHandler.hpp"
#include "security/Crypto.hpp"
#include "utils/Logger.hpp"

using namespace formaia;

int main() {
    utils::Logger::info("FormaIA backend iniciando...");

    // 1) Configuración (variables de entorno / archivo .env)
    auto cfg = config::Config::load(".env");
    if (cfg.groq_api_key.empty() || cfg.groq_api_key == "coloca_aqui_tu_api_key") {
        utils::Logger::warn("GROQ_API_KEY no está configurada. Copia .env.example a .env y "
                             "coloca tu clave de https://console.groq.com antes de generar rutinas.");
    }
    if (cfg.field_encryption_key_base64.empty()) {
        utils::Logger::error("FIELD_ENCRYPTION_KEY_BASE64 no está configurada. El backend no puede "
                              "cifrar datos sensibles. Genera 32 bytes aleatorios en base64, ej.: "
                              "openssl rand -base64 32");
        return 1;
    }

    // 2) io_context: es el reactor/event loop que hace posible la
    //    arquitectura orientada a eventos asíncronos. Se ejecuta con N
    //    hilos (EVENT_WORKER_THREADS) más abajo -ese pool de hilos es el
    //    que procesa: peticiones HTTP entrantes, respuestas de Groq, y
    //    los handlers publicados en el EventBus, todo de forma no
    //    bloqueante y potencialmente en paralelo.
    boost::asio::io_context io;

    // 3) Piezas centrales
    core::EventBus bus(io);
    db::ConnectionPool pool(cfg);
    security::Crypto crypto(cfg.field_encryption_key_base64);
    auto groq = std::make_shared<ia::GroqClient>(io, cfg);

    // 4) Suscripción de handlers: aquí se arma la cadena de eventos
    //    completa del flujo "crear rutina con IA". El orden de
    //    construcción no importa (cada uno solo se suscribe a su
    //    evento), pero conviene leerlos en el orden en que reaccionan:
    ia::IAOrchestratorHandler iaOrchestrator(bus, groq);              // SolicitudIACreada      -> ReferenteInterpretado
    ia::SolicitudEstadoHandler solicitudEstado(bus, pool);            // ReferenteInterpretado / MetaRechazada -> actualiza fila
    ia::MetaSeguridadHandler metaSeguridad(bus, pool, crypto);        // ReferenteInterpretado  -> MetaValidada | MetaRechazada
    rutinas::RutinaBuilderHandler rutinaBuilder(bus);                 // MetaValidada           -> RutinaGenerada
    rutinas::ProgressionHandler progression(bus, pool);               // SesionCompletada       -> UPDATE db
    rutinas::PersistenceHandler persistence(bus, pool);                // RutinaGenerada         -> RutinaPersistida
    notificaciones::NotificationHandler notifications(bus);            // RutinaPersistida / MetaRechazada -> notifica

    // 5) HTTP: las rutas solo insertan datos mínimos y publican eventos;
    //    el trabajo pesado (IA, reglas, progresión, persistencia) ocurre
    //    fuera del hilo que atendió la petición.
    http::Router router;
    http::routes::registrarRutasUsuario(router, pool, crypto, cfg);
    http::routes::registrarRutasSolicitud(router, bus, pool, crypto);
    http::routes::registrarRutasRutina(router, pool);
    http::routes::registrarRutasEjecucion(router, pool);
    http::routes::registrarRutasAlternativas(router, cfg);
    http::routes::registrarRutasEvolucion(router, bus, cfg);
    http::routes::registrarRutasSwagger(router);

    auto server = std::make_shared<http::HttpServer>(io, cfg.http_host,
                                                       static_cast<unsigned short>(cfg.http_port),
                                                       router);
    server->run();
    utils::Logger::info("Escuchando en http://" + cfg.http_host + ":" + std::to_string(cfg.http_port));

    // 6) Apagado ordenado con Ctrl+C / SIGTERM
    boost::asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&io](const boost::system::error_code&, int) {
        utils::Logger::info("Señal de apagado recibida, deteniendo...");
        io.stop();
    });

    // 7) Pool de hilos del reactor: todos corren io.run(), compitiendo
    //    por las tareas encoladas (conexiones HTTP, callbacks de Groq,
    //    handlers de eventos). Este es el núcleo "Event-Driven Async".
    std::vector<std::thread> hilos;
    int n = std::max(1, cfg.event_worker_threads);
    for (int i = 0; i < n - 1; ++i) {
        hilos.emplace_back([&io] { io.run(); });
    }
    io.run(); // el hilo principal también participa del pool

    for (auto& h : hilos) h.join();
    utils::Logger::info("FormaIA backend detenido.");
    return 0;
}
