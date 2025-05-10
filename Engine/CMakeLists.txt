cmake_minimum_required(VERSION 3.30)

add_library(UFox-Engine STATIC
        ufox_graphic.cpp
        ufox_tools_shader_compiler.cpp
        ufox_inputSystem.cpp
        ufox_gui_renderer.cpp
)

target_include_directories(UFox-Engine PUBLIC ${CMAKE_SOURCE_DIR})
