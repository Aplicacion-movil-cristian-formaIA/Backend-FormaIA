#pragma once
#include <sodium.h>
#include <stdexcept>
#include <string>

namespace formaia::security {

// Argon2id vía libsodium (crypto_pwhash_str usa Argon2id por defecto desde
// libsodium 1.0.15). Cumple RNF-05 / sección 8.4: "contraseñas con hash
// (argon2id/bcrypt)". El hash resultante ya incluye la sal y los
// parámetros, así que se guarda tal cual en `usuario.password_hash`.
class PasswordHasher {
public:
    static std::string hash(const std::string& contrasena) {
        char salida[crypto_pwhash_STRBYTES];
        if (crypto_pwhash_str(
                salida, contrasena.c_str(), contrasena.size(),
                crypto_pwhash_OPSLIMIT_INTERACTIVE,
                crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
            throw std::runtime_error("Memoria insuficiente para calcular el hash de la contraseña");
        }
        return std::string(salida);
    }

    static bool verificar(const std::string& contrasena, const std::string& hashGuardado) {
        return crypto_pwhash_str_verify(hashGuardado.c_str(), contrasena.c_str(), contrasena.size()) == 0;
    }
};

} // namespace formaia::security
