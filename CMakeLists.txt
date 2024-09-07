cmake_minimum_required(VERSION 3.12)
project(flash)

if (MSVC)
   add_compile_options("/DUNICODE" "/D_UNICODE")
   add_compile_options("/DNOMINMAX")
endif()

add_executable(flash WIN32 ./main.cpp)

