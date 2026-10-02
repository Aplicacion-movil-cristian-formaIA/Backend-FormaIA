#include <catch2/catch_test_macros.hpp>
#include "security/Crypto.hpp"

using namespace formaia::security;

TEST_CASE("Crypto puede cifrar y descifrar correctamente", "[crypto]") {
    // Generar una llave aleatoria en base64 de 32 bytes para la prueba (simplificada)
    // Usaremos un string hardcodeado válido en base64 de 32 bytes
    std::string keyBase64 = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8="; 
    
    Crypto crypto(keyBase64);

    SECTION("Cifrar un texto simple") {
        std::string texto = "texto secreto 123";
        std::string cifrado = crypto.cifrar(texto);
        
        REQUIRE(cifrado != texto); // Debería haber cambiado
        REQUIRE(!cifrado.empty());

        std::string descifrado = crypto.descifrar(cifrado);
        REQUIRE(descifrado == texto); // Debe ser simétrico
    }

    SECTION("Cifrar un texto vacío") {
        std::string texto = "";
        std::string cifrado = crypto.cifrar(texto);
        std::string descifrado = crypto.descifrar(cifrado);
        REQUIRE(descifrado == texto);
    }
}
