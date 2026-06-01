cmake_minimum_required(VERSION 3.27)
project(sw_battle_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Clang-Tidy integration
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.*")

# Build Options
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

# Project options and strict warnings
add_library(project_options INTERFACE)

target_compile_options(project_options INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
)

if(ENABLE_SANITIZERS)
    target_compile_options(project_options INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(project_options INTERFACE
        -fsanitize=address,undefined
    )
endif()

file(GLOB_RECURSE SOURCES src/*.cpp src/*.hpp)
add_executable(sw_battle_test ${SOURCES})

target_include_directories(sw_battle_test PUBLIC src/)
target_link_libraries(sw_battle_test PRIVATE project_options)
