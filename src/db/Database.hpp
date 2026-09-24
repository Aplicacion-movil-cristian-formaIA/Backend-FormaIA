#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>

#include <mysqlx/xdevapi.h>

#include "config/Config.hpp"

namespace formaia::db {

// Pool sencillo de sesiones X DevAPI. La arquitectura es asíncrona a nivel
// de EventBus/HTTP, pero el conector de MySQL usado aquí (X DevAPI) es
// síncrono por conexión; por eso el pool existe: cada handler "toma
// prestada" una sesión, hace su trabajo (rápido, siempre dentro de un
// handler que ya corre en un hilo del pool de Asio, nunca en el hilo que
// aceptó la petición HTTP) y la devuelve. Así varias operaciones de BD
// pueden avanzar en paralelo sin bloquearse entre sí.
class ConnectionPool {
public:
    ConnectionPool(const config::Config& cfg) : cfg_(cfg) {
        for (int i = 0; i < cfg.db_pool_size; ++i) {
            pool_.push(crearSesion());
        }
    }

    // RAII: al destruirse, la sesión vuelve sola al pool.
    class LeasedSession {
    public:
        LeasedSession(ConnectionPool& owner, std::shared_ptr<mysqlx::Session> session)
            : owner_(owner), session_(std::move(session)) {}
        ~LeasedSession() { if (session_) owner_.devolver(session_); }

        LeasedSession(const LeasedSession&) = delete;
        LeasedSession& operator=(const LeasedSession&) = delete;
        LeasedSession(LeasedSession&&) = default;

        mysqlx::Session& operator*() { return *session_; }
        mysqlx::Session* operator->() { return session_.get(); }

    private:
        ConnectionPool& owner_;
        std::shared_ptr<mysqlx::Session> session_;
    };

    LeasedSession acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&] { return !pool_.empty(); });
        auto session = pool_.front();
        pool_.pop();
        return LeasedSession(*this, session);
    }

private:
    std::shared_ptr<mysqlx::Session> crearSesion() {
        // Nota: X DevAPI habla por el puerto X Protocol (por defecto 33060),
        // NO el 3306 clásico. En XAMPP/MySQL 8+ suele venir habilitado; si
        // no, actívalo con: mysqlx { } en my.ini, o usa mysql_native / el
        // conector clásico (mysqlx::Session también acepta 3306 en MySQL 8
        // vía "auto-detección" en algunas builds -- verifícalo en tu server).
        return std::make_shared<mysqlx::Session>(
            mysqlx::SessionOption::HOST, cfg_.db_host,
            mysqlx::SessionOption::PORT, cfg_.db_port,
            mysqlx::SessionOption::USER, cfg_.db_user,
            mysqlx::SessionOption::PWD, cfg_.db_password,
            mysqlx::SessionOption::DB, cfg_.db_name
        );
    }

    void devolver(std::shared_ptr<mysqlx::Session> session) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.push(std::move(session));
        cv_.notify_one();
    }

    const config::Config& cfg_;
    std::queue<std::shared_ptr<mysqlx::Session>> pool_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace formaia::db
