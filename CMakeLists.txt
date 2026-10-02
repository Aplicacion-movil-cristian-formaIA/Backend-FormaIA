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
list(REMOVE_ITEM FORMAIA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(formaia_core STATIC ${FORMAIA_SOURCES})
target_include_directories(formaia_core PUBLIC src)
target_link_libraries(formaia_core
  PUBLIC
    Boost::system
    Boost::thread
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    unofficial::mysql-connector-cpp::connector
    unofficial-sodium::sodium
)
target_compile_definitions(formaia_core PUBLIC BOOST_ASIO_NO_DEPRECATED)

add_executable(formaia_backend src/main.cpp)
target_link_libraries(formaia_backend PRIVATE formaia_core)

# Tests
find_package(Catch2 CONFIG REQUIRED)
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
if(TEST_SOURCES)
  add_executable(formaia_tests ${TEST_SOURCES})
  target_link_libraries(formaia_tests PRIVATE formaia_core Catch2::Catch2WithMain)
  
  # Incluir soporte CTest (opcional, pero útil)
  include(CTest)
  include(Catch)
  catch_discover_tests(formaia_tests)
endif()

target_compile_definitions(formaia_backend PRIVATE BOOST_ASIO_NO_DEPRECATED)

# En Linux/macOS hace falta pthread para std::thread junto con Boost.Thread
find_package(Threads REQUIRED)
target_link_libraries(formaia_backend PRIVATE Threads::Threads)
