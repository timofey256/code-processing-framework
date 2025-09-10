cmake_minimum_required(VERSION 3.16)
project(http_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

find_path(ASIO_INCLUDE_DIR asio.hpp)
include_directories(${ASIO_INCLUDE_DIR})

add_library(core
    core/dispatcher.cpp
    core/request_parser.cpp
)

add_library(infra
    infra/task_repository.cpp
)

add_executable(server
    app/server.cpp
)

target_link_libraries(server PRIVATE core infra)
