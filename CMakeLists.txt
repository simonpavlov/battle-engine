cmake_minimum_required(VERSION 3.27)
project(sw_battle_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Clang-Tidy integration
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-header-filter=.*")

# Build Options
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)
option(BUILD_VISUALIZER "Build the 3D visualizer" OFF)

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

# Rules engine static library
file(GLOB_RECURSE CORE_SOURCES src/Core/*.cpp)
file(GLOB_RECURSE IO_SOURCES src/IO/*.cpp)
file(GLOB_RECURSE FEATURES_SOURCES src/Features/*.cpp)

add_library(sw_core STATIC ${CORE_SOURCES} ${IO_SOURCES} ${FEATURES_SOURCES})

target_include_directories(sw_core PUBLIC src/)
target_link_libraries(sw_core PRIVATE project_options)

# CLI executable
add_executable(sw_battle_test src/main.cpp)
target_link_libraries(sw_battle_test PRIVATE sw_core project_options)

# 3D visualizer (opt-in)
if(BUILD_VISUALIZER)
    include(FetchContent)

    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
    get_target_property(RAYLIB_INCLUDE_DIRS raylib INTERFACE_INCLUDE_DIRECTORIES)
    if(RAYLIB_INCLUDE_DIRS)
        set_target_properties(raylib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
        target_include_directories(raylib SYSTEM INTERFACE ${RAYLIB_INCLUDE_DIRS})
    endif()

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
    )
    FetchContent_MakeAvailable(imgui)

    FetchContent_Declare(
        rlimgui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(rlimgui)

    add_library(sw_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
        ${rlimgui_SOURCE_DIR}/rlImGui.cpp
    )

    target_include_directories(sw_imgui SYSTEM PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${imgui_SOURCE_DIR}/misc/cpp
        ${rlimgui_SOURCE_DIR}
    )
    target_link_libraries(sw_imgui PUBLIC raylib)
    set_target_properties(sw_imgui PROPERTIES CXX_CLANG_TIDY "")

    file(GLOB_RECURSE GRAPHICS_SOURCES src/Graphics/*.cpp)
    if(GRAPHICS_SOURCES)
        add_executable(sw_visualizer ${GRAPHICS_SOURCES})
        target_include_directories(sw_visualizer PRIVATE
            ${CMAKE_SOURCE_DIR}/src/Graphics
        )
        target_include_directories(sw_visualizer SYSTEM PRIVATE
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
            ${rlimgui_SOURCE_DIR}
        )
        target_link_libraries(sw_visualizer PRIVATE sw_core raylib sw_imgui project_options)

        add_custom_command(TARGET sw_visualizer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                ${CMAKE_SOURCE_DIR}/src/Graphics/assets
                $<TARGET_FILE_DIR:sw_visualizer>/assets
        )
    endif()
endif()
