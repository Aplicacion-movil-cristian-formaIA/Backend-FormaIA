#pragma once
#include <sodium.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace formaia::security {

// Implementa el cifrado de campo descrito en la sección 8.4 del documento
// de requisitos: AES-256-GCM (autenticado) para los campos sensibles
// (correo, fecha de nacimiento, peso, estatura, medidas, lesiones, texto
// libre enviado a la IA), y un HMAC-SHA-256 para el índice de búsqueda
// del correo (permite validar unicidad/hacer login sin descifrar la
// tabla completa).
//
// La clave (32 bytes) se carga UNA vez desde FIELD_ENCRYPTION_KEY_BASE64
// (ver .env.example) y en producción debería salir de un KMS, nunca de
// una variable de entorno plana; aquí se deja así para que el proyecto
// sea ejecutable localmente en XAMPP/desarrollo.
class Crypto {
public:
    explicit Crypto(const std::string& claveBase64) {
        if (sodium_init() < 0) {
            throw std::runtime_error("No se pudo inicializar libsodium");
        }
        clave_ = base64Decode(claveBase64);
        if (clave_.size() != crypto_aead_aes256gcm_KEYBYTES) {
            throw std::runtime_error(
                "FIELD_ENCRYPTION_KEY_BASE64 debe decodificar a exactamente 32 bytes");
        }
        if (crypto_aead_aes256gcm_is_available() == 0) {
            throw std::runtime_error(
                "Esta CPU no soporta aceleración AES-NI requerida por AES-256-GCM en libsodium");
        }
    }

    // Devuelve: base64(nonce || ciphertext || tag). Todo en un solo string
    // para simplificar el almacenamiento en la columna VARBINARY.
    std::string cifrar(const std::string& textoPlano) const {
        std::vector<unsigned char> nonce(crypto_aead_aes256gcm_NPUBBYTES);
        randombytes_buf(nonce.data(), nonce.size());

        std::vector<unsigned char> cifrado(textoPlano.size() + crypto_aead_aes256gcm_ABYTES);
        unsigned long long cifradoLen = 0;

        crypto_aead_aes256gcm_encrypt(
            cifrado.data(), &cifradoLen,
            reinterpret_cast<const unsigned char*>(textoPlano.data()), textoPlano.size(),
            nullptr, 0,          // sin datos asociados adicionales
            nullptr,             // nsec (no se usa en AES-GCM)
            nonce.data(),
            clave_.data());

        std::vector<unsigned char> salida;
        salida.insert(salida.end(), nonce.begin(), nonce.end());
        salida.insert(salida.end(), cifrado.begin(), cifrado.begin() + cifradoLen);
        return base64Encode(salida);
    }

    std::string descifrar(const std::string& base64NonceCifrado) const {
        auto datos = base64Decode(base64NonceCifrado);
        if (datos.size() < crypto_aead_aes256gcm_NPUBBYTES) {
            throw std::runtime_error("Dato cifrado corrupto o demasiado corto");
        }

        std::vector<unsigned char> nonce(datos.begin(), datos.begin() + crypto_aead_aes256gcm_NPUBBYTES);
        std::vector<unsigned char> cifrado(datos.begin() + crypto_aead_aes256gcm_NPUBBYTES, datos.end());

        std::vector<unsigned char> textoPlano(cifrado.size());
        unsigned long long textoPlanoLen = 0;

        int r = crypto_aead_aes256gcm_decrypt(
            textoPlano.data(), &textoPlanoLen,
            nullptr,
            cifrado.data(), cifrado.size(),
            nullptr, 0,
            nonce.data(),
            clave_.data());

        if (r != 0) {
            throw std::runtime_error("Fallo al descifrar: dato manipulado o clave incorrecta");
        }
        return std::string(reinterpret_cast<char*>(textoPlano.data()), textoPlanoLen);
    }

    // Índice ciego para búsquedas (ej. login por correo) sin descifrar la
    // tabla completa. Determinístico: mismo texto de entrada -> mismo hash.
    static std::string hmacIndice(const std::string& texto, const std::string& claveBase64) {
        auto clave = base64Decode(claveBase64);
        unsigned char salida[crypto_auth_hmacsha256_BYTES];
        crypto_auth_hmacsha256(salida,
                                reinterpret_cast<const unsigned char*>(texto.data()), texto.size(),
                                clave.data());
        return toHex(salida, sizeof(salida));
    }

    static std::string base64Encode(const std::vector<unsigned char>& datos) {
        std::string salida(sodium_base64_ENCODED_LEN(datos.size(), sodium_base64_VARIANT_ORIGINAL), '\0');
        sodium_bin2base64(salida.data(), salida.size(), datos.data(), datos.size(),
                           sodium_base64_VARIANT_ORIGINAL);
        salida.resize(std::strlen(salida.c_str()));
        return salida;
    }

    static std::vector<unsigned char> base64Decode(const std::string& texto) {
        std::vector<unsigned char> salida(texto.size());
        size_t binLen = 0;
        if (sodium_base642bin(salida.data(), salida.size(),
                               texto.c_str(), texto.size(),
                               nullptr, &binLen, nullptr,
                               sodium_base64_VARIANT_ORIGINAL) != 0) {
            throw std::runtime_error("Base64 inválido");
        }
        salida.resize(binLen);
        return salida;
    }

private:
    static std::string toHex(const unsigned char* datos, size_t len) {
        static const char* hex = "0123456789abcdef";
        std::string salida(len * 2, '0');
        for (size_t i = 0; i < len; ++i) {
            salida[2 * i] = hex[(datos[i] >> 4) & 0xF];
            salida[2 * i + 1] = hex[datos[i] & 0xF];
        }
        return salida;
    }

    std::vector<unsigned char> clave_;
};

} // namespace formaia::security
