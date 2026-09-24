# Etapa 1: Compilacion (Imagen pesada con compiladores y librerias)
FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-22.04 AS build

WORKDIR /app

# Actualizar e instalar herramientas base
RUN apt-get update && apt-get install -y \
    curl zip unzip tar cmake ninja-build pkg-config

# Clonar e iniciar vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /vcpkg \
    && /vcpkg/bootstrap-vcpkg.sh

# Copiar el codigo fuente de tu backend
COPY . .

# Configurar y compilar con CMake + vcpkg
RUN cmake -B build -S . -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build

# Etapa 2: Imagen de produccion (Muy ligera, solo para ejecutar)
FROM ubuntu:22.04

WORKDIR /app

# Instalar solo certificados SSL (para conectarse a la API de Groq)
RUN apt-get update && apt-get install -y \
    ca-certificates libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Copiar UNICAMENTE el archivo .exe (binario de linux) desde la etapa 1
COPY --from=build /app/build/formaia_backend /app/formaia_backend
# Copiamos tambien tu archivo .env
COPY .env /app/.env

# Exponer el puerto
EXPOSE 8080

# Ejecutar el servidor
CMD ["./formaia_backend"]
