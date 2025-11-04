add_library(boza_core_interface INTERFACE)
add_library(Boza::CoreInterface ALIAS boza_core_interface)

get_source_files(BOZA_CORE_INTERFACE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(boza_core_interface INTERFACE ${BOZA_CORE_INTERFACE_SOURCES}
)

target_include_directories(boza_core_interface
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(spdlog REQUIRED)
find_package(EnTT REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(boza_core_interface
        INTERFACE
        spdlog::spdlog_header_only
        fmt::fmt-header-only
        Boza::Common
        EnTT::EnTT
        nlohmann_json::nlohmann_json
)

add_library(boza_core OBJECT)
add_library(Boza::Core ALIAS boza_core)

get_source_files(BOZA_CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(boza_core PRIVATE ${BOZA_CORE_SOURCES})

set_default_include_dirs(boza_core ${CMAKE_CURRENT_SOURCE_DIR})
boza_enable_warnings(boza_core)


target_link_libraries(boza_core PUBLIC boza_core_interface)

