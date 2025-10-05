add_library(boza_ahi_interface INTERFACE)
get_source_files(BOZA_AHI_INTERFACE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(boza_ahi_interface INTERFACE ${BOZA_AHI_INTERFACE_SOURCES})

target_include_directories(boza_ahi_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(boza_ahi_interface INTERFACE Boza::Common Boza::Core Boza::Platform)


add_library(boza_ahi STATIC)
add_library(Boza::Ahi ALIAS boza_ahi)

target_sources(boza_ahi PRIVATE src/Factory.cpp)

set_default_include_dirs(boza_ahi ${CMAKE_CURRENT_SOURCE_DIR})
boza_enable_warnings(boza_ahi)

target_link_libraries(boza_ahi PUBLIC boza_ahi_interface)


if (BOZA_AUDIO_ENABLED)
    find_package(SndFile REQUIRED)
    target_link_libraries(boza_ahi PRIVATE SndFile::sndfile)

    if (BOZA_ENABLE_OPENAL)
        setup_backend(boza_ahi_openal boza_ahi_interface boza_ahi ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/openal)

        find_package(OpenAL REQUIRED)
        target_link_libraries(boza_ahi_openal PRIVATE OpenAL::OpenAL)
    endif ()

    if (BOZA_ENABLE_XAUDIO2)
        setup_backend(boza_ahi_xaudio2 boza_ahi_interface boza_ahi ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/xaudio2)
    endif ()

    if (BOZA_ENABLE_COREAUDIO)
        setup_backend(boza_ahi_coreaudio boza_ahi_interface boza_ahi ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/coreaudio)
    endif ()
endif ()

if (BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

