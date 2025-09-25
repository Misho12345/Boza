option(BOZA_BUILD_TESTS "Build Boza unit tests" ON)

option(BOZA_ENABLE_OPENGL "Enable OpenGL backend" OFF)
option(BOZA_ENABLE_VULKAN "Enable Vulkan backend" ON)
option(BOZA_ENABLE_METAL "Enable Metal backend (Apple)" OFF)
option(BOZA_ENABLE_DIRECTX11 "Enable DirectX 11 backend (Windows)" OFF)
option(BOZA_ENABLE_DIRECTX12 "Enable DirectX 12 backend (Windows)" OFF)

option(BOZA_ENABLE_OPENAL "Enable OpenAL audio backend" OFF)
option(BOZA_ENABLE_XAUDIO2 "Enable XAudio2 audio backend (Windows)" OFF)
option(BOZA_ENABLE_COREAUDIO "Enable Core Audio backend (macOS)" OFF)

option(BOZA_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(BOZA_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(BOZA_ENABLE_TSAN "Enable ThreadSanitizer" OFF)



if (NOT (BOZA_ENABLE_OPENGL OR BOZA_ENABLE_VULKAN OR BOZA_ENABLE_METAL OR
        BOZA_ENABLE_DIRECTX11 OR BOZA_ENABLE_DIRECTX12))
    message(FATAL_ERROR "At least one graphics backend must be enabled!")
endif ()

if (NOT WIN32 AND BOZA_ENABLE_DIRECTX11)
    message(WARNING "DirectX 11 is only available on Windows, disabling...")
    set(BOZA_ENABLE_DIRECTX11 OFF CACHE BOOL "" FORCE)
endif ()

if (NOT WIN32 AND BOZA_ENABLE_DIRECTX12)
    message(WARNING "DirectX 12 is only available on Windows, disabling...")
    set(BOZA_ENABLE_DIRECTX12 OFF CACHE BOOL "" FORCE)
endif ()

if (NOT APPLE AND (BOZA_ENABLE_METAL))
    message(WARNING "Metal is only available on Apple platforms, disabling...")
    set(BOZA_ENABLE_METAL OFF CACHE BOOL "" FORCE)
endif ()




if (NOT WIN32 AND BOZA_ENABLE_XAUDIO2)
    message(WARNING "XAudio2 is only available on Windows, disabling...")
    set(BOZA_ENABLE_XAUDIO2 OFF CACHE BOOL "" FORCE)
endif ()

if (NOT APPLE AND BOZA_ENABLE_COREAUDIO)
    message(WARNING "Core Audio is only available on Apple platforms, disabling...")
    set(BOZA_ENABLE_COREAUDIO OFF CACHE BOOL "" FORCE)
endif ()

if(NOT (BOZA_ENABLE_OPENAL OR BOZA_ENABLE_XAUDIO2 OR BOZA_ENABLE_COREAUDIO))
    message(WARNING "No audio backend enabled, audio will be disabled")
    set(BOZA_AUDIO_ENABLED OFF)
else()
    set(BOZA_AUDIO_ENABLED ON)
endif()



add_compile_definitions(
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Debug>:BOZA_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CONFIG:Release>:BOZA_RELEASE>

        $<$<BOOL:${BOZA_ENABLE_OPENGL}>:BOZA_OPENGL_ENABLED>
        $<$<BOOL:${BOZA_ENABLE_VULKAN}>:BOZA_VULKAN_ENABLED>
        $<$<BOOL:${BOZA_ENABLE_METAL}>:BOZA_METAL_ENABLED>
        $<$<BOOL:${BOZA_ENABLE_DIRECTX11}>:BOZA_DX11_ENABLED>
        $<$<BOOL:${BOZA_ENABLE_DIRECTX12}>:BOZA_DX12_ENABLED>

        $<$<BOOL:${BOZA_AUDIO_ENABLED}>:BOZA_AUDIO_ENABLED>
        $<$<BOOL:${BOZA_ENABLE_OPENAL}>:BOZA_OPENAL_ENABLED>
        $<$<BOOL:${BOZA_ENABLE_XAUDIO2}>:BOZA_XAUDIO2_ENABLED>
        $<$<BOOL:${BOZA_ENABLE_COREAUDIO}>:BOZA_COREAUDIO_ENABLED>
)



if (NOT MSVC)
    if (BOZA_ENABLE_ASAN)
        message(STATUS "AddressSanitizer enabled")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif ()

    if (BOZA_ENABLE_UBSAN)
        message(STATUS "UndefinedBehaviorSanitizer enabled")
        add_compile_options(-fsanitize=undefined)
        add_link_options(-fsanitize=undefined)
    endif ()

    if (BOZA_ENABLE_TSAN)
        message(STATUS "ThreadSanitizer enabled")
        add_compile_options(-fsanitize=thread)
        add_link_options(-fsanitize=thread)
    endif ()

    if (BOZA_ENABLE_ASAN AND BOZA_ENABLE_TSAN)
        message(FATAL_ERROR "Cannot enable both AddressSanitizer and ThreadSanitizer simultaneously")
    endif ()
else ()
    if (BOZA_ENABLE_ASAN AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.29")
        message(STATUS "AddressSanitizer enabled (MSVC)")
        add_compile_options(/fsanitize=address)
    elseif (BOZA_ENABLE_ASAN)
        message(WARNING "AddressSanitizer requires VS 2019 16.9+ on Windows")
    endif ()
endif ()

if (MSVC)
    add_compile_options(/Zc:preprocessor)

    add_compile_options(
            /Zc:__cplusplus
            /Zc:externConstexpr
            /utf-8
    )

    add_compile_definitions(
            _CRT_SECURE_NO_WARNINGS
            _SCL_SECURE_NO_WARNINGS
    )
endif ()

option(BOZA_ENABLE_LTO "Enable Link Time Optimization" OFF)
if (BOZA_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
    if (lto_supported)
        message(STATUS "Link Time Optimization enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else ()
        message(WARNING "LTO not supported: ${lto_error}")
    endif ()
endif ()

option(BOZA_ENABLE_PROFILING "Enable profiling support" OFF)
if (BOZA_ENABLE_PROFILING)
    add_compile_definitions(BOZA_PROFILING_ENABLED)
    if (NOT MSVC)
        add_compile_options(-pg)
        add_link_options(-pg)
    endif ()
endif ()