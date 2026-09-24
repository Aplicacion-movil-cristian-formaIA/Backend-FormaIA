#pragma once
#include <memory>

#include "core/EventBus.hpp"
#include "domain/events/DomainEvents.hpp"
#include "ia/GroqClient.hpp"
#include "ia/PromptBuilder.hpp"
#include "utils/Logger.hpp"

namespace formaia::ia {

// Se suscribe a SolicitudIACreada. Por cada solicitud:
//   1) Arma el prompt (PromptBuilder) y llama a Groq de forma asíncrona
//      (GroqClient::chatAsync no bloquea; el callback se ejecuta cuando
//      la respuesta HTTP llega, en un hilo del pool de Asio).
//   2) Cuando el callback llega, parsea el JSON y publica
//      ReferenteInterpretado -- el siguiente evento de la cadena.
//   3) Si Groq falla o el JSON no es válido, publica de todas formas un
//      evento con hay_referente=false y una meta por defecto, para que
//      el flujo pueda seguir con una alternativa segura en vez de
//      quedarse "colgado" (degradación controlada, no caída total).
class IAOrchestratorHandler {
public:
    IAOrchestratorHandler(core::EventBus& bus, std::shared_ptr<GroqClient> groq)
        : bus_(bus), groq_(std::move(groq)) {
        bus_.subscribe<domain::events::SolicitudIACreada>(
            [this](const domain::events::SolicitudIACreada& e) { manejar(e); });
    }

private:
    void manejar(const domain::events::SolicitudIACreada& evento) {
        utils::Logger::info("IAOrchestrator: interpretando solicitud " + evento.solicitud_id);

        auto solicitud_id = evento.solicitud_id;
        auto usuario_id = evento.usuario_id;
        auto correlacion_id = evento.correlacion_id;
        auto texto = evento.texto_usuario;

        groq_->chatAsync(
            PromptBuilder::sistemaInterpretacionMeta(),
            texto,
            [this, solicitud_id, usuario_id, correlacion_id](bool exito, const std::string& error, nlohmann::json json) {
                domain::events::ReferenteInterpretado out;
                out.solicitud_id = solicitud_id;
                out.usuario_id = usuario_id;
                out.correlacion_id = correlacion_id;

                if (!exito) {
                    utils::Logger::error("Groq falló para solicitud " + solicitud_id + ": " + error);
                    // Degradación controlada: sin referente, meta genérica de 12 semanas.
                    out.hay_referente = false;
                    out.tipo_meta = "mantener";
                    out.plazo_semanas = 12;
                    out.dias_disponibles = 3;
                    out.arquetipo_nombre = "Acondicionamiento general";
                    bus_.publish(out);
                    return;
                }

                try {
                    out.tipo_meta = json.value("tipo_meta", "mantener");
                    out.plazo_semanas = json.value("plazo_semanas", 12);
                    out.dias_disponibles = json.value("dias_disponibles", 3);
                    out.hay_referente = json.value("hay_referente", false);

                    if (out.hay_referente) {
                        out.referente_nombre = json.value("referente_nombre", "");
                        out.referente_obra = json.value("referente_obra", "");
                        if (json.contains("referente_estatura_cm") && !json["referente_estatura_cm"].is_null())
                            out.referente_estatura_cm = json["referente_estatura_cm"].get<double>();
                        if (json.contains("referente_peso_kg") && !json["referente_peso_kg"].is_null())
                            out.referente_peso_kg = json["referente_peso_kg"].get<double>();
                        out.referente_complexion = json.value("referente_complexion", "");
                        out.fuente_url = json.value("referente_fuente", "conocimiento general del modelo");
                        out.nivel_confianza = json.value("referente_confianza", "no_verificada");
                    }

                    out.arquetipo_nombre = json.value("arquetipo_nombre", "Acondicionamiento general");
                    if (json.contains("arquetipo_enfasis"))
                        out.arquetipo_enfasis_json = json["arquetipo_enfasis"].dump();

                } catch (const std::exception& ex) {
                    utils::Logger::error("JSON de Groq con forma inesperada: " + std::string(ex.what()));
                    out.hay_referente = false;
                    out.tipo_meta = "mantener";
                    out.plazo_semanas = 12;
                    out.dias_disponibles = 3;
                }

                bus_.publish(out);
            });
    }

    core::EventBus& bus_;
    std::shared_ptr<GroqClient> groq_;
};

} // namespace formaia::ia
