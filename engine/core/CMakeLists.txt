add_library(boza_core STATIC)
add_library(Boza::Core ALIAS boza_core)

get_source_files(BOZA_CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(boza_core PRIVATE ${BOZA_CORE_SOURCES})

set_default_include_dirs(boza_core ${CMAKE_CURRENT_SOURCE_DIR})
boza_enable_warnings(boza_core)

find_package(spdlog CONFIG REQUIRED)

target_link_libraries(boza_core
        PUBLIC
        spdlog::spdlog_header_only
        fmt::fmt-header-only
        Boza::Common
)


if(BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
