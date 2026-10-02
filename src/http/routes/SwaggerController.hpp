#pragma once
#include "http/Router.hpp"
#include <string>

namespace formaia::http::routes {

inline void registrarRutasSwagger(Router& router) {
    namespace bhttp = boost::beast::http;

    // Ruta para servir la UI de Swagger usando un CDN público
    router.add(bhttp::verb::get, "/api-docs",
        [](HttpContext&, bhttp::response<bhttp::string_body>& res) {
            std::string html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>FormaIA API Documentation</title>
    <link rel="stylesheet" type="text/css" href="https://cdnjs.cloudflare.com/ajax/libs/swagger-ui/4.15.5/swagger-ui.css" >
    <style>
      html { box-sizing: border-box; overflow: -moz-scrollbars-vertical; overflow-y: scroll; }
      *, *:before, *:after { box-sizing: inherit; }
      body { margin:0; background: #fafafa; }
    </style>
</head>
<body>
    <div id="swagger-ui"></div>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/swagger-ui/4.15.5/swagger-ui-bundle.js"> </script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/swagger-ui/4.15.5/swagger-ui-standalone-preset.js"> </script>
    <script>
    window.onload = function() {
      const ui = SwaggerUIBundle({
        url: "openapi.json",
        dom_id: '#swagger-ui',
        deepLinking: true,
        presets: [
          SwaggerUIBundle.presets.apis,
          SwaggerUIStandalonePreset
        ],
        plugins: [
          SwaggerUIBundle.plugins.DownloadUrl
        ],
        layout: "StandaloneLayout"
      })
      window.ui = ui
    }
  </script>
</body>
</html>
)";
            res.result(bhttp::status::ok);
            res.set(bhttp::field::content_type, "text/html");
            res.body() = html;
        });

    // Ruta para servir el archivo OpenAPI JSON que describe los endpoints
    router.add(bhttp::verb::get, "/openapi.json",
        [](HttpContext&, bhttp::response<bhttp::string_body>& res) {
            std::string json = R"({
  "openapi": "3.0.0",
  "info": {
    "title": "FormaIA Backend API",
    "description": "API del backend en C++ para la plataforma FormaIA.",
    "version": "0.1.0"
  },
  "servers": [
    {
      "url": "http://localhost:8080"
    }
  ],
  "paths": {
    "/api/usuarios": {
      "post": {
        "summary": "Registra un nuevo usuario",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "properties": {
                  "email": {
                    "type": "string",
                    "example": "user@formaia.com"
                  },
                  "password": {
                    "type": "string",
                    "example": "supersecret123"
                  },
                  "fecha_nacimiento": {
                    "type": "string",
                    "example": "1990-05-15"
                  }
                }
              }
            }
          }
        },
        "responses": {
          "201": {
            "description": "Usuario creado exitosamente"
          }
        }
      }
    },
    "/api/usuarios/{id}/perfil": {
      "post": {
        "summary": "Crea o actualiza el perfil físico de un usuario",
        "parameters": [
          {
            "in": "path",
            "name": "id",
            "required": true,
            "schema": {
              "type": "string"
            }
          }
        ],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "properties": {
                  "sexo": {
                    "type": "string",
                    "example": "masculino"
                  },
                  "nivel": {
                    "type": "string",
                    "example": "intermedio"
                  },
                  "dias_semana": {
                    "type": "integer",
                    "example": 4
                  },
                  "minutos_sesion": {
                    "type": "integer",
                    "example": 60
                  },
                  "estatura_cm": {
                    "type": "number",
                    "example": 175.5
                  },
                  "peso_kg": {
                    "type": "number",
                    "example": 72.0
                  },
                  "equipamiento": {
                    "type": "array",
                    "items": {
                      "type": "string"
                    },
                    "example": [
                      "mancuernas",
                      "banco",
                      "barra"
                    ]
                  }
                }
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Perfil actualizado"
          }
        }
      }
    },
    "/api/solicitudes-ia": {
      "post": {
        "summary": "Envía una solicitud en lenguaje natural para generar una rutina con IA",
        "description": "Este endpoint es asíncrono. Retorna inmediatamente un ID de solicitud que luego cambiará su estado.",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "properties": {
                  "usuario_id": {
                    "type": "string",
                    "example": "uuid-del-usuario"
                  },
                  "texto": {
                    "type": "string",
                    "example": "Quiero ganar masa muscular en 3 meses entrenando 4 días a la semana en casa con mancuernas"
                  }
                }
              }
            }
          }
        },
        "responses": {
          "202": {
            "description": "Solicitud aceptada y en procesamiento asíncrono"
          }
        }
      }
    },
    "/api/rutinas/{id}": {
      "get": {
        "summary": "Obtiene los detalles de una rutina generada",
        "parameters": [
          {
            "in": "path",
            "name": "id",
            "required": true,
            "schema": {
              "type": "string"
            }
          }
        ],
        "responses": {
          "200": {
            "description": "Datos de la rutina con sus fases"
          },
          "404": {
            "description": "Rutina no encontrada"
          }
        }
      }
    }
  }
})";
            res.result(bhttp::status::ok);
            res.set(bhttp::field::content_type, "application/json");
            res.body() = json;
        });
}

} // namespace formaia::http::routes
