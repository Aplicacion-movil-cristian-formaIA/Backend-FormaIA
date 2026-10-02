# Etapa 1: Compilacion (Imagen pesada con compiladores y librerias)
FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-22.04 AS build

WORKDIR /app

# Actualizar e instalar herramientas base
RUN apt-get update && apt-get install -y \
    curl zip unzip tar cmake ninja-build pkg-config \
    build-essential autoconf autoconf-archive automake libtool

# Clonar e iniciar vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git /vcpkg \
    && /vcpkg/bootstrap-vcpkg.sh

# Copiar SOLO los archivos necesarios para vcpkg primero
COPY vcpkg.json ./
COPY CMakeLists.txt ./
# Crear archivos falsos para que CMake no arroje error por no tener código fuente
RUN mkdir src && echo "int main(){}" > src/main.cpp && echo "void dummy(){}" > src/dummy.cpp

# Configurar e instalar dependencias con vcpkg ANTES de copiar el resto del código
# Hacemos esto para que Docker guarde la caché de vcpkg de forma permanente
RUN cmake -B build -S . -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Copiar el RESTO del codigo fuente de tu backend (esto sobrescribirá el archivo falso)
COPY . .

# Inyectar dependencias estáticas que faltan para mysql-connector
RUN echo "find_package(Protobuf REQUIRED)" >> CMakeLists.txt && \
    echo "find_package(ZLIB REQUIRED)" >> CMakeLists.txt && \
    echo "find_package(lz4 CONFIG REQUIRED)" >> CMakeLists.txt && \
    echo "find_package(zstd CONFIG REQUIRED)" >> CMakeLists.txt && \
    echo "target_link_libraries(formaia_core PUBLIC protobuf::libprotobuf ZLIB::ZLIB lz4::lz4 \$<TARGET_NAME_IF_EXISTS:zstd::libzstd_shared> \$<TARGET_NAME_IF_EXISTS:zstd::libzstd_static> -lresolv)" >> CMakeLists.txt

# Volver a configurar CMake para que lea todo tu código real
RUN cmake -B build -S . -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Compilar solo el código nuevo
RUN cmake --build build

# Ejecutar las pruebas unitarias y de integración
RUN cd build && ctest --output-on-failure

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
