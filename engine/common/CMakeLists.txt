add_library(boza_common INTERFACE)
add_library(Boza::Common ALIAS boza_common)

get_source_files(BOZA_COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(boza_common INTERFACE ${BOZA_COMMON_SOURCES})

find_package(glm CONFIG REQUIRED)

target_include_directories(boza_common
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_precompile_headers(boza_common INTERFACE include/boza/pch.hpp)
target_link_libraries(boza_common INTERFACE glm::glm-header-only)