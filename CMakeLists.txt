cmake_minimum_required(VERSION 3.30)
project(Boza LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BozaOptions)
include(BozaCompilerWarnings)
include(BozaUtils)

add_subdirectory(engine)
add_subdirectory(game)
