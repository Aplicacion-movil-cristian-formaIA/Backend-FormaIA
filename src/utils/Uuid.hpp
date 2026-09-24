#pragma once
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>

namespace formaia::utils {

// Genera un UUID v4 en el mismo formato que usa MySQL/MariaDB (36 caracteres,
// minúsculas, con guiones). Coincide con el CHAR(36) DEFAULT (UUID()) del
// esquema SQL, así que el backend puede generar el id en C++ o dejar que lo
// genere la base de datos; aquí lo generamos en el backend para poder
// referenciarlo de inmediato en eventos antes de confirmarse en la BD.
inline std::string newUuid() {
    static thread_local boost::uuids::random_generator gen;
    return boost::uuids::to_string(gen());
}

} // namespace formaia::utils
