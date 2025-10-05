add_library(boza_platform STATIC)
add_library(Boza::Platform ALIAS boza_platform)

get_source_files(BOZA_PLATFORM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(boza_platform PRIVATE ${BOZA_PLATFORM_SOURCES})

set_default_include_dirs(boza_platform ${CMAKE_CURRENT_SOURCE_DIR})
boza_enable_warnings(boza_platform)

find_package(glfw3 REQUIRED)

target_link_libraries(boza_platform
        PUBLIC
        Boza::Common
        Boza::Core
        PRIVATE
        glfw
)

if(BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
