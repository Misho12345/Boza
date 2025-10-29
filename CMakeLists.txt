cmake_minimum_required(VERSION 3.24)
project(Boza
        VERSION 1.0.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BozaOptions)
include(BozaCompilerWarnings)
include(BozaUtils)
include(BozaAssets)

add_subdirectory(tools/shader_processor)
add_subdirectory(shaders)

add_subdirectory(engine)
add_subdirectory(game)
