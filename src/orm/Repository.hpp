#pragma once
#include <optional>
#include <string>
#include <vector>

#include <mysqlx/xdevapi.h>

#include "db/Database.hpp"
#include "orm/Column.hpp"

namespace formaia::orm {

// Repository<T> es un repositorio CRUD genérico: cualquier entidad que
// defina su tabla y su lista de Column<T> (ver domain/entities/*.hpp)
// obtiene automáticamente insertar/actualizar/buscarPorId/eliminar sin
// que el código de dominio escriba una sola sentencia SQL.
//
// Ejemplo de uso (dentro de un handler):
//   orm::Repository<Usuario> repo(pool, Usuario::tabla(), Usuario::columnas());
//   repo.insertar(nuevoUsuario);
//   auto u = repo.buscarPorId(id);
template <typename T>
class Repository {
public:
    Repository(db::ConnectionPool& pool, std::string tabla, std::vector<Column<T>> columnas)
        : pool_(pool), tabla_(std::move(tabla)), columnas_(std::move(columnas)) {}

    void insertar(const T& entidad) {
        auto sesion = pool_.acquire();
        mysqlx::Table tabla = sesion->getSchema(sesion->getDefaultSchemaName()).getTable(tabla_);

        std::vector<std::string> nombres;
        for (auto& c : columnas_) nombres.push_back(c.nombre);

        // Nota: la firma exacta de values() en X DevAPI espera los valores
        // como argumentos variádicos o como mysqlx::Row, según la versión
        // del conector. construirValores() arma el vector en el mismo
        // orden que 'nombres'; si tu versión de mysql-connector-cpp exige
        // otra forma de pasar la fila, ajusta solo este método: el resto
        // del ORM (Column<T>, mapearFila) no cambia.
        tabla.insert(nombres).values(construirValores(entidad)).execute();
    }

    void actualizar(const T& entidad, const std::string& idValor) {
        auto sesion = pool_.acquire();
        mysqlx::Table tabla = sesion->getSchema(sesion->getDefaultSchemaName()).getTable(tabla_);
        auto update = tabla.update();
        for (auto& c : columnas_) {
            if (c.esClavePrimaria) continue;
            update = update.set(c.nombre, c.get(entidad));
        }
        std::string idCol = idColumna();
        update.where(idCol + " = :id").bind("id", idValor).execute();
    }

    std::optional<T> buscarPorId(const std::string& id) {
        auto sesion = pool_.acquire();
        mysqlx::Table tabla = sesion->getSchema(sesion->getDefaultSchemaName()).getTable(tabla_);
        std::string idCol = idColumna();

        std::vector<std::string> nombres;
        for (auto& c : columnas_) nombres.push_back(c.nombre);

        auto result = tabla.select(nombres)
                          .where(idCol + " = :id")
                          .bind("id", id)
                          .execute();

        mysqlx::Row fila = result.fetchOne();
        if (!fila) return std::nullopt;
        return mapearFila(fila);
    }

    std::vector<T> buscarTodosPor(const std::string& condicionSql,
                                   const std::string& valor) {
        auto sesion = pool_.acquire();
        mysqlx::Table tabla = sesion->getSchema(sesion->getDefaultSchemaName()).getTable(tabla_);

        std::vector<std::string> nombres;
        for (auto& c : columnas_) nombres.push_back(c.nombre);

        auto result = tabla.select(nombres)
                          .where(condicionSql)
                          .bind("valor", valor)
                          .execute();

        std::vector<T> salida;
        for (mysqlx::Row fila : result.fetchAll()) {
            salida.push_back(mapearFila(fila));
        }
        return salida;
    }

    void eliminar(const std::string& id) {
        auto sesion = pool_.acquire();
        mysqlx::Table tabla = sesion->getSchema(sesion->getDefaultSchemaName()).getTable(tabla_);
        std::string idCol = idColumna();
        tabla.remove().where(idCol + " = :id").bind("id", id).execute();
    }

private:
    std::string idColumna() const {
        for (auto& c : columnas_) if (c.esClavePrimaria) return c.nombre;
        throw std::runtime_error("La entidad de la tabla '" + tabla_ +
                                  "' no tiene columna marcada como clave primaria");
    }

    std::vector<mysqlx::Value> construirValores(const T& entidad) {
        std::vector<mysqlx::Value> valores;
        for (auto& c : columnas_) valores.push_back(c.get(entidad));
        return valores;
    }

    T mapearFila(mysqlx::Row& fila) {
        T entidad{};
        for (size_t i = 0; i < columnas_.size(); ++i) {
            columnas_[i].set(entidad, fila[i]);
        }
        return entidad;
    }

    db::ConnectionPool& pool_;
    std::string tabla_;
    std::vector<Column<T>> columnas_;
};

} // namespace formaia::orm
