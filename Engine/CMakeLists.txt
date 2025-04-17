cmake_minimum_required(VERSION 3.30)

# Create a target for the library (e.g., a static library)
add_library(UFox-Graphic-Device STATIC)
target_compile_features(UFox-Graphic-Device PUBLIC cxx_std_23
)

# Make the library's include directories public to dependents
target_include_directories(UFox-Graphic-Device PUBLIC ${CMAKE_SOURCE_DIR})

# Specify modules using target_sources with the CXX_MODULES file set
target_sources(UFox-Graphic-Device
        PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_SOURCE_DIR}"
        FILES "ufox_graphic_device.ixx"
)

