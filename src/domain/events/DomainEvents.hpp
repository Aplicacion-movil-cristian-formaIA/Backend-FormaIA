#pragma once
#include <string>
#include <vector>
#include <optional>

// -----------------------------------------------------------------------
// Eventos de dominio del flujo "crear rutina con IA" (HU-04 a HU-09).
//
// Cadena normal (camino feliz):
//   SolicitudIACreada
//     -> (IAOrchestratorHandler llama a Groq, interpreta meta/referente)
//   ReferenteInterpretado           (si el usuario mencionó un personaje)
//     -> (ReglasSeguridad valida la meta contra límites saludables)
//   MetaValidada
//     -> (RutinaBuilderHandler arma fases/semanas progresivas)
//   RutinaGenerada
//     -> (PersistenceHandler guarda todo con el ORM)
//   RutinaPersistida
//     -> (NotificationHandler notifica al usuario / actualiza el estado)
//
// Camino alterno (meta insegura, RF-09 / RNF-07):
//   SolicitudIACreada -> ... -> MetaRechazada -> (se guarda el motivo)
// -----------------------------------------------------------------------

namespace formaia::domain::events {

// Se publica en cuanto el usuario envía su meta en lenguaje natural y la
// fila ya quedó insertada en `solicitud_ia` (estado = 'aclaracion').
struct SolicitudIACreada {
    std::string solicitud_id;
    std::string usuario_id;
    std::string texto_usuario;      // Prompt original, tal como lo escribió
    std::string correlacion_id;     // Para seguir el flujo completo en logs
    std::string perfil_fisico_json; // Contexto extra para que la IA se adapte al usuario
};

// Se publica cuando la IA (Groq) detectó y "buscó" un referente/personaje
// en el mensaje del usuario y lo tradujo a una ficha + arquetipo.
struct ReferenteInterpretado {
    std::string solicitud_id;
    std::string usuario_id;
    std::string correlacion_id;

    bool hay_referente = false;
    std::string referente_nombre;      // Ej. "Kirito"
    std::string referente_obra;        // Ej. "Sword Art Online"
    std::optional<double> referente_estatura_cm;
    std::optional<double> referente_peso_kg;
    std::string referente_complexion;
    std::string fuente_url;
    std::string nivel_confianza;       // oficial | wiki | no_verificada

    std::string arquetipo_nombre;      // Ej. "Atlético delgado y definido"
    std::string arquetipo_enfasis_json;// JSON: ["espalda","hombros","core"]

    // Meta ya extraída del texto libre (independiente del referente)
    std::string tipo_meta;             // bajar_peso | ganar_masa | definir | mantener
    int plazo_semanas = 12;
    int dias_disponibles = 3;
};

// Se publica cuando ReglasSeguridad (código determinístico, no la IA)
// aprueba la meta calculada para el cuerpo real del usuario.
struct MetaValidada {
    std::string solicitud_id;
    std::string usuario_id;
    std::string correlacion_id;

    std::string tipo_meta;
    double peso_actual_kg = 0;
    double peso_objetivo_kg = 0;
    int plazo_semanas = 12;
    int dias_disponibles = 3;
    std::string arquetipo_nombre;
    std::vector<std::string> zonas_lesionadas;   // se excluyen ejercicios de estas zonas
    std::string equipamiento;                    // sin_equipo | mancuernas | gimnasio
};

// Se publica cuando ReglasSeguridad RECHAZA la meta (RF-09).
struct MetaRechazada {
    std::string solicitud_id;
    std::string usuario_id;
    std::string correlacion_id;
    std::string motivo;
    double sugerencia_peso_kg = 0;
    int sugerencia_plazo_semanas = 0;
};

// Estructura intermedia: una sesión dentro de una semana, ya con sus
// ejercicios elegidos (ids que existen en la tabla `ejercicio`).
struct SesionPlanDTO {
    int semana = 1;
    int dia_semana = 1;          // 1=lunes .. 7=domingo
    int duracion_min = 30;
    bool es_descarga = false;
    std::vector<std::string> ejercicio_ids;
};

struct FaseDTO {
    int orden = 1;
    std::string nombre;          // Adaptación | Desarrollo | Consolidación
    int semana_inicio = 1;
    int semana_fin = 4;
    std::string objetivo;
    std::vector<SesionPlanDTO> sesiones;
};

// Se publica cuando RutinaBuilderHandler terminó de armar la progresión
// completa (RF-10 / RF-11), lista para persistir.
struct RutinaGenerada {
    std::string solicitud_id;
    std::string usuario_id;
    std::string correlacion_id;
    std::string nombre;
    int semanas_totales = 12;
    std::vector<FaseDTO> fases;
};

// Se publica cuando PersistenceHandler ya guardó la rutina completa con
// el ORM (rutina + fase + sesion_plan + sesion_ejercicio).
struct RutinaPersistida {
    std::string rutina_id;
    std::string usuario_id;
    std::string solicitud_id;
    std::string correlacion_id;
};

// Se publica cuando el usuario termina una sesión de entrenamiento (RPE feedback)
struct SesionCompletada {
    std::string sesion_id;
    std::string usuario_id;
    int rpe; // Rating of Perceived Exertion (1 a 10)
};

} // namespace formaia::domain::events
