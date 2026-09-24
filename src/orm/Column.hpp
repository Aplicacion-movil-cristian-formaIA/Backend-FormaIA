#pragma once
#include <functional>
#include <string>
#include <mysqlx/xdevapi.h>

namespace formaia::orm {

// ---------------------------------------------------------------------
// ORM ligero de FormaIA
// ---------------------------------------------------------------------
// Es un mapeador objeto-relacional propio (no un paquete de terceros como
// ODB o sqlpp11) porque ambos exigen un compilador/generador de código
// aparte y no se puede validar esa cadena de build en este entorno sin
// red. Este ORM es más simple, pero cumple lo esencial de un ORM:
//   - Cada entidad C++ (Usuario, Rutina, ...) se mapea a una tabla.
//   - Cada Column<T> mapea UN atributo de la entidad a UNA columna,
//     con un getter y un setter (lambdas), sin escribir SQL a mano en
//     el código de dominio.
//   - Repository<T> (ver Repository.hpp) genera el INSERT/UPDATE/SELECT
//     genérico a partir de esas columnas.
// Si más adelante quieres migrar a ODB o sqlpp11, solo se reemplaza esta
// carpeta orm/ — el resto del backend (eventos, handlers, HTTP) no
// depende del mecanismo interno del ORM.
//
// Column<T> representa una columna de la tabla mapeada a un campo de la
// entidad T. `set` guarda el valor leído de MySQL en el objeto C++;
// `get` construye el valor de MySQL a partir del objeto C++.
template <typename T>
struct Column {
    std::string nombre;
    bool esClavePrimaria = false;

    std::function<mysqlx::Value(const T&)> get;
    std::function<void(T&, const mysqlx::Value&)> set;
};

} // namespace formaia::orm
