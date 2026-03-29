cmake_minimum_required(VERSION 3.30...4.1 FATAL_ERROR)

# === C++ Standard Configuration ===
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_MODULE_STD 1)

# === Experimental C++ Import Std ===
if (CMAKE_VERSION VERSION_LESS_EQUAL "3.31.7")
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
elseif (CMAKE_VERSION VERSION_GREATER_EQUAL "4.0.0" AND CMAKE_VERSION VERSION_LESS_EQUAL "4.0.2")
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
else ()
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
endif ()

# === Project Declaration ===
project(Boza
        VERSION 1.0.0
        LANGUAGES CXX
)

# === CMake Modules ===
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BozaOptions)
include(BozaCompilerWarnings)
include(BozaUtils)
include(BozaAssets)
include(BozaHeaderUnits)
include(BozaHeaderPatches)
boza_enable_vcpkg_header_fixes()
include(BozaExecutable)

# === Subdirectories ===
add_subdirectory(tools/shader_processor)
add_subdirectory(shaders)
add_subdirectory(boza)
add_subdirectory(examples/material_showcase)
add_subdirectory(examples/instancing_example)
#add_subdirectory(game) NOT WORKING, NEEDS FIXING

