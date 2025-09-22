cmake_minimum_required(VERSION 3.16)
project(http_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_path(ASIO_INCLUDE_DIR asio.hpp)
include_directories(${ASIO_INCLUDE_DIR})

add_library(core INTERFACE
)

add_library(infra
    infra/task_repository.cpp
    infra/user_repository.cpp
    infra/dispatcher.cpp
    infra/request_parser.cpp
)

add_library(lib
    lib/random.cpp
    lib/env.cpp
)

add_library(services
    services/auth_service.cpp
)
target_link_libraries(services PRIVATE infra)

add_executable(server
    app/server.cpp
)

target_link_libraries(server PRIVATE core infra services lib OpenSSL::Crypto nlohmann_json::nlohmann_json)
