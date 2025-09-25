add_library(boza_ahi STATIC)
add_library(Boza::Ahi ALIAS boza_ahi)

get_source_files(BOZA_AHI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(boza_ahi PRIVATE ${BOZA_AHI_SOURCES})

set_default_include_dirs(boza_ahi ${CMAKE_CURRENT_SOURCE_DIR})
boza_enable_warnings(boza_ahi)

find_package(SndFile REQUIRED)

target_link_libraries(boza_ahi
        PRIVATE
        Boza::Common
        Boza::Core
        Boza::Platform
        SndFile::sndfile
)

if (BOZA_ENABLE_OPENAL)
    find_package(OpenAL REQUIRED)
    target_link_libraries(boza_ahi PRIVATE OpenAL::OpenAL)

    target_precompile_headers(boza_ahi PRIVATE src/backends/openal/pch.hpp)
endif ()

if (BOZA_ENABLE_XAUDIO2)
    target_precompile_headers(boza_ahi PRIVATE src/backends/openal/pch.hpp)
endif ()

if (BOZA_ENABLE_COREAUDIO)
    target_precompile_headers(boza_ahi PRIVATE src/backends/openal/pch.hpp)
endif ()

if (BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

