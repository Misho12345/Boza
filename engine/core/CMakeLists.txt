add_library(boza_core STATIC)
add_library(Boza::Core ALIAS boza_core)

get_source_files(BOZA_CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(boza_core PRIVATE ${BOZA_CORE_SOURCES})

set_default_include_dirs(boza_core ${CMAKE_CURRENT_SOURCE_DIR})
boza_enable_warnings(boza_core)

#find_package(unofficial-spirv-reflect CONFIG REQUIRED)
#find_package(unofficial-shaderc CONFIG REQUIRED)
#find_package(SPIRV-Tools CONFIG REQUIRED)
#find_package(SPIRV-Tools-opt CONFIG REQUIRED)
#find_package(EnTT CONFIG REQUIRED)

#find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
#find_package(magic_enum CONFIG REQUIRED)
#find_package(Boost REQUIRED COMPONENTS pfr preprocessor)

target_link_libraries(boza_core
        PUBLIC
#        glm::glm-header-only
        fmt::fmt-header-only
        spdlog::spdlog_header_only
#        magic_enum::magic_enum
#        Boost::pfr
#        Boost::preprocessor
        PRIVATE
        Boza::Common
        #        unofficial::spirv-reflect
        #        unofficial::shaderc::shaderc
        #        SPIRV-Tools-static
        #        SPIRV-Tools-opt
)


if(BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
