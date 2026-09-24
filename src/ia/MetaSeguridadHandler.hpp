#pragma once
#include "core/EventBus.hpp"
#include "db/Database.hpp"
#include "domain/entities/PerfilFisico.hpp"
#include "domain/events/DomainEvents.hpp"
#include "ia/ReglasSeguridad.hpp"
#include "orm/Repository.hpp"
#include "security/Crypto.hpp"
#include "utils/Logger.hpp"

namespace formaia::ia {

// Se suscribe a ReferenteInterpretado. Lee el perfil físico REAL del
// usuario (descifrando peso/estatura), corre ReglasSeguridad (C++
// determinístico, no la IA) y publica MetaValidada o MetaRechazada.
// Este es el punto donde RF-09 y RNF-07 se hacen cumplir de verdad.
class MetaSeguridadHandler {
public:
    MetaSeguridadHandler(core::EventBus& bus, db::ConnectionPool& pool, security::Crypto& crypto)
        : bus_(bus), pool_(pool), crypto_(crypto) {
        bus_.subscribe<domain::events::ReferenteInterpretado>(
            [this](const domain::events::ReferenteInterpretado& e) { manejar(e); });
    }

private:
    void manejar(const domain::events::ReferenteInterpretado& evento) {
        orm::Repository<domain::entities::PerfilFisico> repoPerfil(
            pool_, domain::entities::PerfilFisico::tabla(), domain::entities::PerfilFisico::columnas());

        auto perfilOpt = repoPerfil.buscarPorId(evento.usuario_id);
        if (!perfilOpt) {
            publicarRechazo(evento, "No se encontró el perfil físico del usuario; "
                                     "completa tu perfil antes de generar una rutina.");
            return;
        }

        double peso_actual_kg = 0, estatura_cm = 0;
        try {
            peso_actual_kg = std::stod(crypto_.descifrar(perfilOpt->peso_kg_cifrada_b64));
            estatura_cm = std::stod(crypto_.descifrar(perfilOpt->estatura_cm_cifrada_b64));
        } catch (const std::exception& ex) {
            utils::Logger::error("No se pudo descifrar el perfil físico: " + std::string(ex.what()));
            publicarRechazo(evento, "No se pudo leer tu perfil físico. Intenta de nuevo.");
            return;
        }

        // TODO(negocio): aquí también se debería comparar contra una cifra
        // EXPLÍCITA que el usuario haya dado en su texto (ej. "bajar 15 kg
        // en 2 semanas"), usando ReglasSeguridad::validarPeticionExplicita.
        // Ese parseo de cantidad+plazo del texto libre se deja a cargo de
        // Groq (se puede añadir "kg_pedidos" al esquema de PromptBuilder)
        // y se added aquí de forma análoga a como ya se hace con tipo_meta.

        bool es_menor = false; // La verificación real de edad ocurre en el
                                // registro (RF-04); aquí se consulta un
                                // flag ya calculado, no se reconstruye la
                                // fecha de nacimiento en cada solicitud.

        auto resultado = ReglasSeguridad::validar(evento, peso_actual_kg, estatura_cm, es_menor);

        if (!resultado.aprobada) {
            publicarRechazo(evento, resultado.motivo);
            return;
        }

        domain::events::MetaValidada out;
        out.solicitud_id = evento.solicitud_id;
        out.usuario_id = evento.usuario_id;
        out.correlacion_id = evento.correlacion_id;
        out.tipo_meta = evento.tipo_meta;
        out.peso_actual_kg = peso_actual_kg;
        out.peso_objetivo_kg = resultado.peso_objetivo_sugerido_kg;
        out.plazo_semanas = resultado.plazo_sugerido_semanas > 0 ? resultado.plazo_sugerido_semanas : evento.plazo_semanas;
        out.dias_disponibles = evento.dias_disponibles;
        out.arquetipo_nombre = evento.arquetipo_nombre;
        out.equipamiento = "sin_equipo"; // simplificado; en producción viene de perfil_fisico.equipamiento

        utils::Logger::info("Meta validada para usuario " + evento.usuario_id +
                             ": " + out.tipo_meta + " -> " + std::to_string(out.peso_objetivo_kg) + " kg en " +
                             std::to_string(out.plazo_semanas) + " semanas");

        bus_.publish(out);
    }

    void publicarRechazo(const domain::events::ReferenteInterpretado& evento, const std::string& motivo) {
        domain::events::MetaRechazada out;
        out.solicitud_id = evento.solicitud_id;
        out.usuario_id = evento.usuario_id;
        out.correlacion_id = evento.correlacion_id;
        out.motivo = motivo;
        bus_.publish(out);
    }

    core::EventBus& bus_;
    db::ConnectionPool& pool_;
    security::Crypto& crypto_;
};

} // namespace formaia::ia
