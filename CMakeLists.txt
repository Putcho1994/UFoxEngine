cmake_minimum_required(VERSION 3.30)



project(UFoxEngine  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")



add_executable(UFoxEngine)
target_compile_features(UFoxEngine PUBLIC cxx_std_23
)
target_sources(UFoxEngine
        PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES main.ixx
)

add_library(UFox-Util STATIC)

target_compile_features(UFox-Util PUBLIC cxx_std_23
)

# Make the library's include directories public to dependents
target_include_directories(UFox-Util PUBLIC ${CMAKE_SOURCE_DIR})

# Specify modules using target_sources with the CXX_MODULES file set
target_sources(UFox-Util
        PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_SOURCE_DIR}"
        FILES "ufox_utils.ixx"
)


# CPM setup
file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.7/CPM.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake"
        EXPECTED_HASH SHA256=c0fc82149e00c43a21febe7b2ca57b2ffea2b8e88ab867022c21d6b81937eb50
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage(NAME SDL3 GITHUB_REPOSITORY libsdl-org/SDL GIT_TAG release-3.2.10 OPTIONS "SDL_VULKAN ON" "SDL_STATIC OFF")
CPMAddPackage(NAME fmt GITHUB_REPOSITORY fmtlib/fmt GIT_TAG 11.1.4)
CPMAddPackage(NAME Vulkan-Headers GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers GIT_TAG v1.4.312)


# set up Vulkan C++ module as a library
add_library(VulkanHppModule )
target_sources(VulkanHppModule
        PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS "${Vulkan-Headers_SOURCE_DIR}/include"
        FILES "${Vulkan-Headers_SOURCE_DIR}/include/vulkan/vulkan.cppm"
)

target_compile_definitions(VulkanHppModule PUBLIC
        # Your options here, project-dependent:
        # https://github.com/KhronosGroup/Vulkan-Hpp#configuration-options
        "VULKAN_HPP_NO_SMART_HANDLE=ON"
        "VULKAN_HPP_NO_EXCEPTIONS=ON"
        "VULKAN_HPP_NO_CONSTRUCTORS=ON"
        "VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=OFF"

)

target_compile_features(VulkanHppModule PUBLIC cxx_std_23)
target_link_libraries(VulkanHppModule PUBLIC Vulkan-Headers )

add_subdirectory(Windowing)
add_subdirectory(Engine)



add_definitions(-DUFOX_DEBUG)

set(LIBS)
list(APPEND LIBS VulkanHppModule SDL3::SDL3 fmt::fmt)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS} UFox-Windowing UFox-Graphic-Device UFox-Util)
target_link_libraries(UFox-Windowing PRIVATE  ${LIBS} UFox-Util)
target_link_libraries(UFox-Graphic-Device PRIVATE ${LIBS} UFox-Windowing UFox-Util)
target_link_libraries(UFox-Util PRIVATE ${LIBS})








