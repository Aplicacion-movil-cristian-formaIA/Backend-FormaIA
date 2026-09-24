cmake_minimum_required(VERSION 3.22)
project(formaia_backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)

file(GLOB_RECURSE FORMAIA_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(formaia_backend ${FORMAIA_SOURCES})

target_include_directories(formaia_backend PRIVATE src)

target_link_libraries(formaia_backend
  PRIVATE
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    unofficial::mysql-connector-cpp::connector
    unofficial-sodium::sodium
)

target_compile_definitions(formaia_backend PRIVATE BOOST_ASIO_NO_DEPRECATED)

# En Linux/macOS hace falta pthread para std::thread junto con Boost.Thread
find_package(Threads REQUIRED)
target_link_libraries(formaia_backend PRIVATE Threads::Threads)
