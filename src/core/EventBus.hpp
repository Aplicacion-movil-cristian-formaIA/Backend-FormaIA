#pragma once
#include <any>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include "utils/Logger.hpp"

namespace formaia::core {

// ---------------------------------------------------------------------
// EventBus
// ---------------------------------------------------------------------
// Es el corazón de la arquitectura orientada a eventos asíncronos:
//
//   1) Cualquier parte del sistema (un controlador HTTP, un handler, etc.)
//      llama a bus.publish<MiEvento>(evento).
//   2) publish() NO ejecuta a los suscriptores en el mismo hilo/llamada:
//      encola cada handler en el io_context con boost::asio::post(), y
//      retorna de inmediato. Quien publica nunca espera a que el trabajo
//      termine (por eso un controlador HTTP puede responder 202 Accepted
//      al instante mientras la IA sigue trabajando en segundo plano).
//   3) El io_context se ejecuta con un pool de hilos (ver main.cpp,
//      io_context.run() llamado N veces en N hilos), así que los handlers
//      de distintos eventos -e incluso del mismo evento- pueden correr en
//      paralelo, sin bloquear el hilo que aceptó la petición HTTP.
//   4) Un handler, al terminar su trabajo, típicamente publica un nuevo
//      evento (ej. SolicitudIACreada -> ... -> RutinaGenerada -> ...
//      -> RutinaPersistida), formando una cadena de reacciones: es el
//      patrón Event-Driven / reactor clásico.
//
// El despacho se indexa por std::type_index del evento (cada struct de
// evento de dominio en domain/events/DomainEvents.hpp es un tipo distinto),
// así que suscribirse es 100% tipado en tiempo de compilación:
//
//   bus.subscribe<SolicitudIACreada>([](const SolicitudIACreada& e) {...});
//
class EventBus {
public:
    explicit EventBus(boost::asio::io_context& io) : io_(io) {}

    template <typename EventT>
    using Handler = std::function<void(const EventT&)>;

    // Registra un handler para el tipo de evento EventT. Se pueden
    // registrar varios handlers para el mismo evento (todos se ejecutan).
    template <typename EventT>
    void subscribe(Handler<EventT> handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& vec = handlersFor<EventT>();
        vec.push_back(std::move(handler));
    }

    // Publica un evento: encola de forma asíncrona la ejecución de cada
    // handler suscrito. No bloquea al llamador.
    template <typename EventT>
    void publish(EventT event) {
        std::vector<Handler<EventT>> handlersCopy;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            handlersCopy = handlersFor<EventT>();
        }

        utils::Logger::info("Evento publicado: " + std::string(typeid(EventT).name()) +
                             " (" + std::to_string(handlersCopy.size()) + " suscriptor(es))");

        for (auto& handler : handlersCopy) {
            // Copiamos el evento por cada handler porque se ejecutan de
            // forma asíncrona y potencialmente concurrente.
            boost::asio::post(io_, [handler, event]() {
                try {
                    handler(event);
                } catch (const std::exception& ex) {
                    utils::Logger::error(std::string("Handler de evento lanzó excepción: ") + ex.what());
                } catch (...) {
                    utils::Logger::error("Handler de evento lanzó una excepción desconocida");
                }
            });
        }
    }

private:
    template <typename EventT>
    std::vector<Handler<EventT>>& handlersFor() {
        auto key = std::type_index(typeid(EventT));
        auto it = handlers_.find(key);
        if (it == handlers_.end()) {
            it = handlers_.emplace(key, std::vector<Handler<EventT>>{}).first;
        }
        return std::any_cast<std::vector<Handler<EventT>>&>(it->second);
    }

    boost::asio::io_context& io_;
    std::mutex mutex_;
    std::unordered_map<std::type_index, std::any> handlers_;
};

} // namespace formaia::core
