# FormaIA backend (C++)

Backend en C++20 para la app de rutinas de ejercicio con IA: arquitectura
**orientada a eventos asíncronos**, **ORM ligero propio** sobre MySQL/MariaDB
(XAMPP), e integración con **Groq** para interpretar la meta del usuario en
lenguaje natural.

> ⚠️ **Importante — léelo antes de compilar:** este código se escribió en un
> entorno sin acceso a internet ni a un compilador C++/toolchain, así que
> **no se pudo compilar ni ejecutar aquí**. Es un scaffold completo y
> coherente (arquitectura, flujo de eventos, mapeo ORM, llamada a Groq,
> persistencia), pero es muy probable que necesites ajustes menores al
> compilarlo por primera vez: la API exacta de `mysqlx` (X DevAPI) varía
> ligeramente entre versiones de `mysql-connector-cpp`, por ejemplo. Dejé
> comentarios en el código (`Repository.hpp`, `Database.hpp`) señalando los
> puntos más sensibles a la versión de la librería.

## Arquitectura

```
Petición HTTP (POST /api/solicitudes-ia)
        │
        ▼
SolicitudController: guarda la fila inicial y publica SolicitudIACreada
        │  (el hilo HTTP responde 202 Accepted AQUÍ, sin esperar nada más)
        ▼
EventBus (boost::asio::post -> ejecución asíncrona en el pool de hilos)
        │
        ▼
IAOrchestratorHandler ── llama a Groq (async, Boost.Beast) ──► ReferenteInterpretado
        │
        ▼
MetaSeguridadHandler ── reglas de seguridad en C++ (RF-09/RNF-07) ──► MetaValidada | MetaRechazada
        │
        ▼
RutinaBuilderHandler ── arma fases y progresión (RF-10/RF-11) ──► RutinaGenerada
        │
        ▼
PersistenceHandler ── guarda con el ORM (rutina/fase/sesion_plan/sesion_ejercicio) ──► RutinaPersistida
        │
        ▼
NotificationHandler ── notifica al usuario (hoy: log; extensible a push/WebSocket)
```

Cada flecha es un **evento publicado en el `EventBus`**, no una llamada de
función directa: cada handler solo conoce el evento al que se suscribe, no a
quién lo publicó ni quién lo va a consumir después. Eso es lo que permite
que el hilo que recibió la petición HTTP quede libre de inmediato (responde
`202 Accepted`) mientras Groq, las reglas de seguridad y la persistencia
ocurren en segundo plano, en el pool de hilos que corre `io_context::run()`
(ver `main.cpp`, `EVENT_WORKER_THREADS`).

El cliente consulta el resultado con `GET /api/solicitudes-ia/{id}` (polling
simple). El código deja marcado con `TODO` dónde conectar un WebSocket si
prefieres actualización push en vez de polling.

## Por qué un ORM propio y no ODB/sqlpp11

Ambas son librerías C++ serias, pero requieren un generador de código aparte
(`odb` compiler, `sqlpp11-ddl2cpp`) que no se puede ejecutar ni validar en
este entorno. El ORM en `src/orm/` es deliberadamente simple pero cumple lo
esencial: cada entidad (`src/domain/entities/*.hpp`) declara su tabla y sus
columnas como `Column<T>` (getter/setter), y `Repository<T>` genera
`insertar`, `actualizar`, `buscarPorId`, `buscarTodosPor` y `eliminar` de
forma genérica, sin SQL escrito a mano en el código de negocio. Si más
adelante prefieres ODB o sqlpp11, solo se reemplaza la carpeta `orm/`; el
resto del backend no depende de su mecanismo interno.

**Entidades ya mapeadas:** `Usuario`, `PerfilFisico`, `SolicitudIA`,
`Rutina`, `Fase`, `SesionPlan`, `Ejercicio`, `SesionEjercicio`. El esquema
SQL (`sql/formaia_schema.sql`) tiene más tablas (`Limitacion`,
`ArquetipoFisico`, `ReferenteFicha`, `Entrenamiento`, `RegistroSerie`,
`Medicion`, `Recordatorio`, `Valoracion`); se dejaron fuera de este entregable
para no disparar el tamaño del código, pero se mapean con el **mismo patrón
exacto** que las ocho ya escritas (copia un archivo de `domain/entities/`,
cambia el nombre de tabla y las columnas).

## Sobre la búsqueda de la ficha del personaje (Kirito, etc.)

Groq (como la mayoría de APIs de chat completions) **no navega internet por
sí solo**: responde con el conocimiento de su entrenamiento. En
`PromptBuilder.hpp` se le pide explícitamente que, si no está seguro de los
datos del personaje, devuelva `referente_confianza: "no_verificada"` y deje
los números en `null` en vez de inventarlos — pero eso no es lo mismo que
una búsqueda real y verificada como describe el documento de requisitos
(RF-08).

Para una búsqueda real en internet, la forma limpia de extenderlo es:
1. Crear una interfaz `IReferenceLookupService` (buscar nombre → ficha).
2. Implementarla con una API de búsqueda web (ej. Tavily, Serper, Bing
   Search API) llamada de forma asíncrona igual que `GroqClient`.
3. En `IAOrchestratorHandler`, antes de pedirle la interpretación a Groq (o
   como *function calling*/*tool use* de Groq, que sí soporta llamado a
   herramientas), llamar primero a esa interfaz y pasarle el resultado a
   Groq como contexto adicional en el prompt.

Así el dato que se guarda en `referente_ficha` (la caché, ver el esquema
SQL) es trazable a una fuente real, tal como pide el documento de
requisitos, y Groq deja de tener que "recordar" medidas exactas.

## Compilar (Linux/macOS/WSL, con vcpkg)

```bash
git clone https://github.com/microsoft/vcpkg
./vcpkg/bootstrap-vcpkg.sh

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build -j
```

Esto instala (vía `vcpkg.json`): Boost.Asio/Beast, OpenSSL, nlohmann-json,
mysql-connector-cpp y libsodium.

En Windows, usa vcpkg igual pero con `vcpkg install --triplet x64-windows` y
abre la carpeta como proyecto CMake en Visual Studio.

## Configurar y correr contra XAMPP

1. Importa el esquema en phpMyAdmin o por consola:
   ```bash
   mysql -u root -p < sql/formaia_schema.sql
   ```
2. Copia `.env.example` a `.env` y completa:
   - `DB_*`: normalmente `DB_HOST=127.0.0.1`, `DB_USER=root`, `DB_PASSWORD=`
     (XAMPP trae MySQL sin contraseña por defecto).
   - `GROQ_API_KEY`: créala en https://console.groq.com
   - `FIELD_ENCRYPTION_KEY_BASE64`: genera 32 bytes aleatorios, ej.:
     ```bash
     openssl rand -base64 32
     ```
3. **Importante — puerto de MySQL:** este backend usa el **X DevAPI**
   (`mysqlx`), que habla por el **X Protocol** (puerto `33060` por defecto),
   no el puerto clásico `3306`. En MySQL 8 suele venir habilitado por
   defecto; en XAMPP/MariaDB puede no estarlo. Si tu conexión falla,
   revisa `SHOW VARIABLES LIKE 'mysqlx%';` en tu servidor, o considera
   cambiar `db/Database.hpp` para usar el conector clásico
   (`mysqlx::Session` con el puerto 3306 en despliegues MySQL 8 recientes,
   o la API `MYSQL*` de libmysqlclient si te quedas en MariaDB puro).
4. Ejecuta:
   ```bash
   ./build/formaia_backend
   ```

## Endpoints incluidos (MVP)

| Método | Ruta | Qué hace |
|---|---|---|
| POST | `/api/usuarios` | Registro (RF-01): cifra correo y fecha de nacimiento, hashea la contraseña |
| POST | `/api/usuarios/{id}/perfil` | Guarda perfil físico (RF-03): cifra peso y estatura |
| POST | `/api/solicitudes-ia` | Crea la solicitud y dispara el flujo de eventos (RF-06). Responde `202` de inmediato |
| GET | `/api/solicitudes-ia/{id}` | Consulta el estado (`aclaracion`/`rechazada`/`generada`) — polling |
| GET | `/api/rutinas/{id}` | Devuelve la rutina generada con sus fases |
| GET | `/api/rutinas/por-solicitud/{solicitud_id}` | Devuelve el `rutina_id` generado a partir del id de una solicitud (para que el cliente pase de "solicitud" a "rutina" tras el polling) |

**No incluidos en este MVP** (quedan como siguiente paso, siguiendo el mismo
patrón arquitectónico): login/JWT, sesión guiada de entrenamiento, registro
de series, progreso/mediciones, notificaciones push, panel admin, y el
endpoint de "ajustar rutina en lenguaje natural" (RF-12).

## Ejemplo de flujo completo (curl)

```bash
# 1) Registro
curl -X POST localhost:8080/api/usuarios -d '{
  "email":"alex@correo.com","password":"unaClaveSegura123",
  "fecha_nacimiento":"1996-03-14"}'

# 2) Perfil físico
curl -X POST localhost:8080/api/usuarios/<id>/perfil -d '{
  "sexo":"masculino","estatura_cm":175,"peso_kg":78.4,
  "nivel":"principiante","dias_semana":4}'

# 3) Pedir la rutina con IA
curl -X POST localhost:8080/api/solicitudes-ia -d '{
  "usuario_id":"<id>",
  "texto":"Quiero bajar de peso y verme como Kirito, tengo 4 dias a la semana"}'
# -> 202 { "id": "<solicitud_id>", "estado": "procesando", ... }

# 4) Consultar el resultado (repetir hasta que estado = "generada")
curl localhost:8080/api/solicitudes-ia/<solicitud_id>
```
