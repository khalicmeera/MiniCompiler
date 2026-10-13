cmake_minimum_required(VERSION 3.16)
project(MiniCompiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(minic src/main.cpp src/ir.cpp)
if(MSVC)
    target_compile_options(minic PRIVATE /W4)
else()
    target_compile_options(minic PRIVATE -Wall -Wextra -Wpedantic)
endif()
