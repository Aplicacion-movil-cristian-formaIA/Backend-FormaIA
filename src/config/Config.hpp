#pragma once
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace formaia::config {

// Config centraliza toda la configuración del backend. Se carga una sola
// vez al arrancar (main.cpp) y se pasa por referencia const a quien la
// necesite; así evitamos leer variables de entorno dispersas por el código.
struct Config {
    std::string http_host = "0.0.0.0";
    int http_port = 8080;
    int event_worker_threads = 4;

    std::string db_host = "127.0.0.1";
    int db_port = 3306;
    std::string db_name = "formaia";
    std::string db_user = "root";
    std::string db_password;
    int db_pool_size = 8;

    std::string groq_api_key;
    std::string groq_model = "llama-3.3-70b-versatile";
    std::string groq_api_host = "api.groq.com";
    int groq_timeout_ms = 20000;

    std::string field_encryption_key_base64;
    std::string jwt_secret;

    // Carga variables desde un archivo .env (formato KEY=VALUE, líneas que
    // empiezan con # se ignoran) y luego permite que las variables de
    // entorno reales del sistema sobreescriban lo leído del archivo.
    static Config load(const std::string& env_path = ".env") {
        std::unordered_map<std::string, std::string> values;

        std::ifstream file(env_path);
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            values[key] = val;
        }

        auto get = [&](const char* key, const std::string& def) -> std::string {
            if (const char* env = std::getenv(key)) return std::string(env);
            auto it = values.find(key);
            if (it != values.end()) return it->second;
            return def;
        };
        auto getInt = [&](const char* key, int def) -> int {
            std::string v = get(key, "");
            if (v.empty()) return def;
            try { return std::stoi(v); } catch (...) { return def; }
        };

        Config c;
        c.http_host = get("HTTP_HOST", c.http_host);
        c.http_port = getInt("HTTP_PORT", c.http_port);
        c.event_worker_threads = getInt("EVENT_WORKER_THREADS", c.event_worker_threads);

        c.db_host = get("DB_HOST", c.db_host);
        c.db_port = getInt("DB_PORT", c.db_port);
        c.db_name = get("DB_NAME", c.db_name);
        c.db_user = get("DB_USER", c.db_user);
        c.db_password = get("DB_PASSWORD", c.db_password);
        c.db_pool_size = getInt("DB_POOL_SIZE", c.db_pool_size);

        c.groq_api_key = get("GROQ_API_KEY", c.groq_api_key);
        c.groq_model = get("GROQ_MODEL", c.groq_model);
        c.groq_api_host = get("GROQ_API_HOST", c.groq_api_host);
        c.groq_timeout_ms = getInt("GROQ_TIMEOUT_MS", c.groq_timeout_ms);

        c.field_encryption_key_base64 = get("FIELD_ENCRYPTION_KEY_BASE64", "");
        c.jwt_secret = get("JWT_SECRET", "");

        return c;
    }
};

} // namespace formaia::config
