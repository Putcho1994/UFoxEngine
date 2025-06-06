cmake_minimum_required(VERSION 3.14)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Add a custom command to create the Contents folder and copy its contents
add_custom_command(
        OUTPUT "${CMAKE_SOURCE_DIR}/bin/Contents"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/bin/Contents"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Contents" "${CMAKE_SOURCE_DIR}/bin/Contents"
        DEPENDS "${CMAKE_SOURCE_DIR}/Contents"
        COMMENT "Creating and copying Contents folder to bin/Contents"
)

# Add a custom target to ensure the copy happens
add_custom_target(CopyContents ALL DEPENDS "${CMAKE_SOURCE_DIR}/bin/Contents")


project(UFoxEngine LANGUAGES CXX)

# CPM setup
file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.7/CPM.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake"
        EXPECTED_HASH SHA256=c0fc82149e00c43a21febe7b2ca57b2ffea2b8e88ab867022c21d6b81937eb50
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage(NAME SDL3 GITHUB_REPOSITORY libsdl-org/SDL GIT_TAG release-3.2.12 OPTIONS "SDL_VULKAN ON" "SDL_STATIC OFF")
CPMAddPackage(NAME fmt GITHUB_REPOSITORY fmtlib/fmt GIT_TAG 11.1.4)
CPMAddPackage(NAME Vulkan-Headers GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers GIT_TAG v1.4.312)
CPMAddPackage(NAME glm GITHUB_REPOSITORY g-truc/glm GIT_TAG 1.0.1)

CPMAddPackage(NAME SDL3_image GITHUB_REPOSITORY libsdl-org/SDL_image GIT_TAG release-3.2.4)

target_compile_definitions(Vulkan-Headers INTERFACE
        "VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=OFF"
)

set(LIBS)
list(APPEND LIBS Vulkan-Headers SDL3::SDL3 fmt::fmt glm::glm SDL3_image::SDL3_image)

add_subdirectory(Windowing)
add_subdirectory(Engine)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS} UFox-Windowing UFox-Engine)
target_link_libraries(UFox-Windowing PRIVATE ${LIBS})
target_link_libraries(UFox-Engine PRIVATE ${LIBS} UFox-Windowing)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${PROJECT_SOURCE_DIR}/Shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/Shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/Shaders/*.comp"
)

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
    message(STATUS "BUILDING SHADER")
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_SOURCE_DIR}/bin/Shaders/${FILE_NAME}.spv")
    message(STATUS ${GLSL})
    ##execute glslang command to compile that specific shader
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)


add_custom_target(
        shaders
        ALL DEPENDS ${SPIRV_BINARY_FILES}
)


add_dependencies(${PROJECT_NAME} shaders CopyContents)








