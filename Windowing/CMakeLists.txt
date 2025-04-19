cmake_minimum_required(VERSION 3.30)

add_library(UFox-Windowing STATIC ufox_windowing.cpp)

target_include_directories(UFox-Windowing PUBLIC ${CMAKE_SOURCE_DIR})
