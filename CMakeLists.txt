cmake_minimum_required(VERSION 3.12)
project(flash)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (WIN32)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(UNICODE)
    add_compile_definitions(_UNICODE)
endif()

add_executable(flash WIN32 ./main.cpp)

