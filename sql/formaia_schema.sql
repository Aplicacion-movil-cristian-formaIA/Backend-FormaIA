-- =====================================================================
-- FormaIA · Esquema de base de datos (MySQL / MariaDB - XAMPP)
-- =====================================================================
-- Motor: InnoDB (transacciones y llaves foráneas)
-- Codificación: utf8mb4 (emojis, tildes, ñ)
-- Requiere: MySQL 8.0+ o MariaDB 10.4+ (funciones UUID, CHECK, columnas
--           generadas). En XAMPP moderno (MariaDB 10.4/10.6) funciona tal cual.
--
-- Cómo usarlo en XAMPP:
--   1) Abre phpMyAdmin -> pestaña "SQL" (o usa la consola: mysql -u root -p)
--   2) Pega y ejecuta este archivo completo (crea la BD y todas las tablas)
--
-- NOTA SOBRE CIFRADO (ver sección 8.4 del documento de requisitos):
--   MySQL no cifra campos automáticamente. Los campos marcados como
--   sensibles se definen como VARBINARY: la aplicación (backend) los
--   cifra con AES-256-GCM ANTES de insertarlos y los descifra al leer.
--   La clave de cifrado NUNCA se guarda en esta base de datos.
--   Se deja también, comentada, la alternativa AES_ENCRYPT/AES_DECRYPT
--   nativa de MySQL por si se prefiere cifrar en la propia consulta
--   (opción más simple pero con manejo de clave más débil; úsala solo
--   en desarrollo/pruebas, no en producción).
-- =====================================================================

DROP DATABASE IF EXISTS formaia;
CREATE DATABASE formaia
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;
USE formaia;

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 1;

-- =====================================================================
-- 1) USUARIO Y PERFIL
-- =====================================================================

CREATE TABLE usuario (
  id                  CHAR(36)      NOT NULL DEFAULT (UUID()),
  email               VARBINARY(255) NOT NULL COMMENT 'Cifrado en la app (AES-256-GCM)',
  email_idx           CHAR(64)      NOT NULL COMMENT 'HMAC-SHA-256 del correo en minúsculas: permite buscar/validar unicidad sin descifrar',
  password_hash       VARCHAR(255)  NULL COMMENT 'argon2id/bcrypt; NULL si el login es social',
  proveedor_auth      ENUM('email','google','apple') NOT NULL DEFAULT 'email',
  fecha_nacimiento    VARBINARY(64) NOT NULL COMMENT 'Cifrada en la app',
  rol                 ENUM('usuario','admin') NOT NULL DEFAULT 'usuario',
  acepto_terminos_en  DATETIME      NOT NULL,
  acepto_salud_en     DATETIME      NULL COMMENT 'Consentimiento de datos de salud (RF-25)',
  creado_en           DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP,
  actualizado_en      DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  eliminado_en        DATETIME      NULL COMMENT 'Borrado lógico',
  PRIMARY KEY (id),
  UNIQUE KEY uq_usuario_email_idx (email_idx)
) ENGINE=InnoDB;

CREATE TABLE perfil_fisico (
  usuario_id       CHAR(36)      NOT NULL,
  sexo             ENUM('femenino','masculino','prefiero_no_decir') NULL,
  estatura_cm      VARBINARY(64) NULL COMMENT 'Cifrado en la app; valor original SMALLINT 100-250',
  peso_kg          VARBINARY(64) NULL COMMENT 'Cifrado en la app; valor original DECIMAL(5,2) > 0',
  nivel            ENUM('principiante','intermedio','avanzado') NOT NULL DEFAULT 'principiante',
  dias_semana      TINYINT       NOT NULL DEFAULT 3,
  minutos_sesion   SMALLINT      NOT NULL DEFAULT 30,
  equipamiento     JSON          NULL COMMENT 'Ej. ["sin_equipo","mancuernas"]',
  actualizado_en   DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (usuario_id),
  CONSTRAINT fk_perfil_usuario FOREIGN KEY (usuario_id)
    REFERENCES usuario (id) ON DELETE CASCADE,
  CONSTRAINT chk_dias_semana CHECK (dias_semana BETWEEN 1 AND 7)
) ENGINE=InnoDB;

CREATE TABLE limitacion (
  id            CHAR(36)      NOT NULL DEFAULT (UUID()),
  usuario_id    CHAR(36)      NOT NULL,
  zona          VARCHAR(50)   NOT NULL COMMENT 'Rodilla, hombro, espalda baja...',
  descripcion   VARBINARY(500) NULL COMMENT 'Cifrado en la app: texto libre de la lesión',
  creado_en     DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY ix_limitacion_usuario (usuario_id),
  CONSTRAINT fk_limitacion_usuario FOREIGN KEY (usuario_id)
    REFERENCES usuario (id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- =====================================================================
-- 2) ARQUETIPOS Y REFERENTES (personajes)
-- =====================================================================

CREATE TABLE arquetipo_fisico (
  id                    CHAR(36)     NOT NULL DEFAULT (UUID()),
  nombre                VARCHAR(80)  NOT NULL COMMENT 'Ej. "Atlético delgado definido"',
  descripcion           TEXT         NULL,
  enfasis_muscular      JSON         NULL COMMENT 'Ej. ["espalda","hombros","core"]',
  rango_grasa_corporal  VARCHAR(30)  NULL COMMENT 'Ej. "12-15%"',
  tipo_entrenamiento    JSON         NULL COMMENT 'Ej. {"fuerza":60,"cardio":30,"movilidad":10}',
  aprobado_por_admin    TINYINT(1)   NOT NULL DEFAULT 0,
  creado_en             DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id)
) ENGINE=InnoDB;

CREATE TABLE referente_ficha (
  id                CHAR(36)     NOT NULL DEFAULT (UUID()),
  nombre            VARCHAR(100) NOT NULL COMMENT 'Ej. "Kirito"',
  obra              VARCHAR(120) NULL COMMENT 'Serie, novela o juego de origen',
  estatura_cm       SMALLINT     NULL COMMENT 'Dato factual público, no sensible del usuario',
  peso_kg           DECIMAL(5,2) NULL,
  complexion        TEXT         NULL COMMENT 'Descripción textual del físico',
  fuente_url        VARCHAR(500) NOT NULL,
  nivel_confianza   ENUM('oficial','wiki','no_verificada') NOT NULL DEFAULT 'wiki',
  arquetipo_id      CHAR(36)     NULL,
  consultado_en     DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uq_referente_nombre_obra (nombre, obra),
  KEY ix_referente_arquetipo (arquetipo_id),
  CONSTRAINT fk_referente_arquetipo FOREIGN KEY (arquetipo_id)
    REFERENCES arquetipo_fisico (id) ON DELETE SET NULL
) ENGINE=InnoDB COMMENT='Caché de fichas técnicas de personajes; solo datos factuales y enlace a la fuente';

-- =====================================================================
-- 3) SOLICITUDES A LA IA Y RUTINAS
-- =====================================================================

CREATE TABLE solicitud_ia (
  id                    CHAR(36)     NOT NULL DEFAULT (UUID()),
  usuario_id            CHAR(36)     NOT NULL,
  texto_usuario         VARBINARY(2000) NOT NULL COMMENT 'Cifrado en la app: prompt original',
  referente_detectado   VARCHAR(100) NULL,
  referente_ficha_id    CHAR(36)     NULL,
  arquetipo_id          CHAR(36)     NULL,
  meta_extraida         JSON         NULL COMMENT 'Meta, plazo, preferencias (sin datos sensibles en claro)',
  estado                ENUM('aclaracion','rechazada','generada','error') NOT NULL DEFAULT 'aclaracion',
  motivo_rechazo        TEXT         NULL,
  modelo_ia             VARCHAR(50)  NULL,
  creado_en             DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY ix_solicitud_usuario (usuario_id),
  CONSTRAINT fk_solicitud_usuario FOREIGN KEY (usuario_id)
    REFERENCES usuario (id) ON DELETE CASCADE,
  CONSTRAINT fk_solicitud_ficha FOREIGN KEY (referente_ficha_id)
    REFERENCES referente_ficha (id) ON DELETE SET NULL,
  CONSTRAINT fk_solicitud_arquetipo FOREIGN KEY (arquetipo_id)
    REFERENCES arquetipo_fisico (id) ON DELETE SET NULL
) ENGINE=InnoDB;

CREATE TABLE rutina (
  id               CHAR(36)     NOT NULL DEFAULT (UUID()),
  usuario_id       CHAR(36)     NOT NULL,
  solicitud_id     CHAR(36)     NULL,
  nombre           VARCHAR(120) NOT NULL,
  semanas_totales  SMALLINT     NOT NULL DEFAULT 12,
  activa           TINYINT(1)   NOT NULL DEFAULT 1,
  -- columna generada: solo tiene valor cuando la rutina está activa;
  -- el índice único de abajo garantiza como máximo UNA rutina activa por usuario
  usuario_si_activa CHAR(36) GENERATED ALWAYS AS (IF(activa = 1, usuario_id, NULL)) STORED,
  version          INT          NOT NULL DEFAULT 1,
  creado_en        DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY ix_rutina_usuario (usuario_id),
  UNIQUE KEY uq_rutina_activa_por_usuario (usuario_si_activa),
  CONSTRAINT fk_rutina_usuario FOREIGN KEY (usuario_id)
    REFERENCES usuario (id),
  CONSTRAINT fk_rutina_solicitud FOREIGN KEY (solicitud_id)
    REFERENCES solicitud_ia (id) ON DELETE SET NULL
) ENGINE=InnoDB;

CREATE TABLE fase (
  id             CHAR(36)     NOT NULL DEFAULT (UUID()),
  rutina_id      CHAR(36)     NOT NULL,
  orden          TINYINT      NOT NULL,
  nombre         VARCHAR(60)  NOT NULL COMMENT 'Adaptación / Desarrollo / Consolidación',
  semana_inicio  SMALLINT     NOT NULL,
  semana_fin     SMALLINT     NOT NULL,
  objetivo       TEXT         NULL,
  PRIMARY KEY (id),
  KEY ix_fase_rutina (rutina_id),
  CONSTRAINT fk_fase_rutina FOREIGN KEY (rutina_id)
    REFERENCES rutina (id) ON DELETE CASCADE,
  CONSTRAINT chk_fase_semanas CHECK (semana_fin >= semana_inicio)
) ENGINE=InnoDB;

CREATE TABLE sesion_plan (
  id            CHAR(36)   NOT NULL DEFAULT (UUID()),
  fase_id       CHAR(36)   NOT NULL,
  semana        SMALLINT   NOT NULL,
  dia_semana    TINYINT    NOT NULL COMMENT '1=lunes ... 7=domingo',
  duracion_min  SMALLINT   NOT NULL,
  es_descarga   TINYINT(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  KEY ix_sesionplan_fase (fase_id),
  CONSTRAINT fk_sesionplan_fase FOREIGN KEY (fase_id)
    REFERENCES fase (id) ON DELETE CASCADE,
  CONSTRAINT chk_dia_semana CHECK (dia_semana BETWEEN 1 AND 7)
) ENGINE=InnoDB;

-- =====================================================================
-- 4) EJERCICIOS
-- =====================================================================

CREATE TABLE ejercicio (
  id                  CHAR(36)     NOT NULL DEFAULT (UUID()),
  nombre              VARCHAR(120) NOT NULL,
  grupo_muscular      VARCHAR(50)  NOT NULL,
  equipamiento        VARCHAR(50)  NOT NULL DEFAULT 'sin_equipo',
  nivel               ENUM('principiante','intermedio','avanzado') NOT NULL DEFAULT 'principiante',
  video_url           VARCHAR(500) NULL,
  contraindicaciones  JSON         NULL COMMENT 'Ej. ["rodilla","hombro"]',
  activo              TINYINT(1)   NOT NULL DEFAULT 1,
  PRIMARY KEY (id),
  KEY ix_ejercicio_grupo (grupo_muscular)
) ENGINE=InnoDB;

CREATE TABLE sesion_ejercicio (
  id                CHAR(36)     NOT NULL DEFAULT (UUID()),
  sesion_plan_id    CHAR(36)     NOT NULL,
  ejercicio_id      CHAR(36)     NOT NULL,
  orden             TINYINT      NOT NULL,
  series            TINYINT      NOT NULL,
  repeticiones      VARCHAR(20)  NOT NULL COMMENT 'Ej. "8-12" o "40 s"',
  descanso_seg      SMALLINT     NOT NULL DEFAULT 60,
  carga_sugerida    VARCHAR(30)  NULL,
  PRIMARY KEY (id),
  KEY ix_sesej_sesion (sesion_plan_id),
  KEY ix_sesej_ejercicio (ejercicio_id),
  CONSTRAINT fk_sesej_sesion FOREIGN KEY (sesion_plan_id)
    REFERENCES sesion_plan (id) ON DELETE CASCADE,
  CONSTRAINT fk_sesej_ejercicio FOREIGN KEY (ejercicio_id)
    REFERENCES ejercicio (id) ON DELETE RESTRICT
) ENGINE=InnoDB;

-- =====================================================================
-- 5) ENTRENAMIENTOS Y REGISTRO
-- =====================================================================

CREATE TABLE entrenamiento (
  id              CHAR(36)   NOT NULL DEFAULT (UUID()),
  usuario_id      CHAR(36)   NOT NULL,
  sesion_plan_id  CHAR(36)   NOT NULL,
  iniciado_en     DATETIME   NOT NULL DEFAULT CURRENT_TIMESTAMP,
  finalizado_en   DATETIME   NULL,
  completado      TINYINT(1) NOT NULL DEFAULT 0,
  rpe             TINYINT    NULL COMMENT 'Esfuerzo percibido 1-10',
  PRIMARY KEY (id),
  KEY ix_entrenamiento_usuario_fecha (usuario_id, iniciado_en),
  KEY ix_entrenamiento_sesion (sesion_plan_id),
  CONSTRAINT fk_entrenamiento_usuario FOREIGN KEY (usuario_id)
    REFERENCES usuario (id) ON DELETE CASCADE,
  CONSTRAINT fk_entrenamiento_sesion FOREIGN KEY (sesion_plan_id)
    REFERENCES sesion_plan (id) ON DELETE CASCADE,
  CONSTRAINT chk_rpe CHECK (rpe IS NULL OR rpe BETWEEN 1 AND 10)
) ENGINE=InnoDB;

CREATE TABLE registro_serie (
  id                    CHAR(36)      NOT NULL DEFAULT (UUID()),
  entrenamiento_id      CHAR(36)      NOT NULL,
  ejercicio_id          CHAR(36)      NOT NULL COMMENT 'Puede ser un sustituto del ejercicio planificado',
  numero_serie          TINYINT       NOT NULL,
  repeticiones_hechas   SMALLINT      NULL,
  carga_kg              DECIMAL(6,2)  NULL,
  PRIMARY KEY (id),
  KEY ix_registro_entrenamiento (entrenamiento_id),
  CONSTRAINT fk_registro_entrenamiento FOREIGN KEY (entrenamiento_id)
    REFERENCES entrenamiento (id) ON DELETE CASCADE,
  CONSTRAINT fk_registro_ejercicio FOREIGN KEY (ejercicio_id)
    REFERENCES ejercicio (id) ON DELETE RESTRICT
) ENGINE=InnoDB;

-- =====================================================================
-- 6) PROGRESO, NOTIFICACIONES Y VALORACIONES
-- =====================================================================

CREATE TABLE medicion (
  id           CHAR(36)      NOT NULL DEFAULT (UUID()),
  usuario_id   CHAR(36)      NOT NULL,
  fecha        DATE          NOT NULL,
  peso_kg      VARBINARY(32) NULL COMMENT 'Cifrado en la app',
  cintura_cm   VARBINARY(32) NULL COMMENT 'Cifrado en la app',
  pecho_cm     VARBINARY(32) NULL COMMENT 'Cifrado en la app',
  brazo_cm     VARBINARY(32) NULL COMMENT 'Cifrado en la app',
  foto_url     VARCHAR(500)  NULL COMMENT 'Objeto cifrado en el almacenamiento (SSE-KMS); URL firmada temporal',
  creado_en    DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY ix_medicion_usuario_fecha (usuario_id, fecha),
  CONSTRAINT fk_medicion_usuario FOREIGN KEY (usuario_id)
    REFERENCES usuario (id) ON DELETE CASCADE
) ENGINE=InnoDB;

CREATE TABLE recordatorio (
  id            CHAR(36)   NOT NULL DEFAULT (UUID()),
  usuario_id    CHAR(36)   NOT NULL,
  dia_semana    TINYINT    NOT NULL,
  hora          TIME       NOT NULL,
  activo        TINYINT(1) NOT NULL DEFAULT 1,
  PRIMARY KEY (id),
  KEY ix_recordatorio_usuario (usuario_id),
  CONSTRAINT fk_recordatorio_usuario FOREIGN KEY (usuario_id)
    REFERENCES usuario (id) ON DELETE CASCADE,
  CONSTRAINT chk_recordatorio_dia CHECK (dia_semana BETWEEN 1 AND 7)
) ENGINE=InnoDB;

CREATE TABLE valoracion (
  id             CHAR(36)   NOT NULL DEFAULT (UUID()),
  rutina_id      CHAR(36)   NOT NULL,
  usuario_id     CHAR(36)   NOT NULL,
  calificacion   TINYINT    NOT NULL,
  comentario     TEXT       NULL,
  reportada      TINYINT(1) NOT NULL DEFAULT 0,
  motivo_reporte TEXT       NULL,
  creado_en      DATETIME   NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY ix_valoracion_rutina (rutina_id),
  CONSTRAINT fk_valoracion_rutina FOREIGN KEY (rutina_id)
    REFERENCES rutina (id) ON DELETE CASCADE,
  CONSTRAINT fk_valoracion_usuario FOREIGN KEY (usuario_id)
    REFERENCES usuario (id) ON DELETE CASCADE,
  CONSTRAINT chk_calificacion CHECK (calificacion BETWEEN 1 AND 5)
) ENGINE=InnoDB;

-- =====================================================================
-- 7) DATOS INICIALES MÍNIMOS (opcional, útil para probar en XAMPP)
-- =====================================================================

INSERT INTO arquetipo_fisico (nombre, descripcion, enfasis_muscular, rango_grasa_corporal, tipo_entrenamiento, aprobado_por_admin)
VALUES
 ('Atlético delgado y definido', 'Complexión ligera con definición muscular moderada.',
  JSON_ARRAY('espalda','hombros','core'), '12-15%',
  JSON_OBJECT('fuerza',60,'cardio',30,'movilidad',10), 1),
 ('Musculoso voluminoso', 'Mayor masa muscular general, énfasis en volumen.',
  JSON_ARRAY('pecho','espalda','piernas'), '15-18%',
  JSON_OBJECT('fuerza',75,'cardio',15,'movilidad',10), 1);

INSERT INTO ejercicio (nombre, grupo_muscular, equipamiento, nivel, contraindicaciones)
VALUES
 ('Flexiones de pecho', 'pecho', 'sin_equipo', 'principiante', JSON_ARRAY('muñeca')),
 ('Sentadilla con peso corporal', 'piernas', 'sin_equipo', 'principiante', JSON_ARRAY('rodilla')),
 ('Plancha abdominal', 'core', 'sin_equipo', 'principiante', JSON_ARRAY()),
 ('Remo con mancuerna', 'espalda', 'mancuernas', 'intermedio', JSON_ARRAY('espalda_baja')),
 ('Zancadas', 'piernas', 'sin_equipo', 'intermedio', JSON_ARRAY('rodilla'));

-- =====================================================================
-- 8) EJEMPLO de cifrado/descifrado nativo de MySQL (SOLO referencia)
-- =====================================================================
-- La aplicación (backend) debe hacer el cifrado con AES-256-GCM y una
-- clave gestionada por un KMS (ver sección 8.4 del documento). El
-- siguiente ejemplo es solo para pruebas locales en phpMyAdmin/XAMPP,
-- NO para producción (clave fija en la consulta):
--
-- INSERT INTO usuario (email, email_idx, password_hash, fecha_nacimiento, acepto_terminos_en)
-- VALUES (
--   AES_ENCRYPT('alex@correo.com', 'clave-temporal-de-prueba'),
--   SHA2(LOWER('alex@correo.com'), 256),
--   '$argon2id$...hash...',
--   AES_ENCRYPT('1996-03-14', 'clave-temporal-de-prueba'),
--   NOW()
-- );
--
-- SELECT CAST(AES_DECRYPT(email, 'clave-temporal-de-prueba') AS CHAR)
-- FROM usuario;
-- =====================================================================

-- Fin del script
