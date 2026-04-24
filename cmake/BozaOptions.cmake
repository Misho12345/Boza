include_guard(GLOBAL)

# === Build Options ===
option(BOZA_BUILD_TESTS "Build Boza unit tests" OFF)
option(BOZA_USE_MIMALLOC "Use mimalloc for memory allocation" ON)
option(BOZA_ENABLE_PROFILING "Enable profiling support" OFF)

# === Graphics Backends ===
option(BOZA_ENABLE_OPENGL "Enable OpenGL backend" OFF)
option(BOZA_ENABLE_VULKAN "Enable Vulkan backend" ON)
option(BOZA_ENABLE_METAL "Enable Metal backend (Apple)" OFF)
option(BOZA_ENABLE_DIRECTX11 "Enable DirectX 11 backend (Windows)" OFF)
option(BOZA_ENABLE_DIRECTX12 "Enable DirectX 12 backend (Windows)" OFF)

# === Audio Backends ===
option(BOZA_ENABLE_OPENAL "Enable OpenAL audio backend" OFF)
option(BOZA_ENABLE_XAUDIO2 "Enable XAudio2 audio backend (Windows)" OFF)
option(BOZA_ENABLE_COREAUDIO "Enable Core Audio backend (macOS)" OFF)

# === Sanitizers ===
option(BOZA_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(BOZA_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(BOZA_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# === Validation ===
function(_validate_backends)
    # Graphics validation
    if (NOT (BOZA_ENABLE_OPENGL OR BOZA_ENABLE_VULKAN OR BOZA_ENABLE_METAL OR
            BOZA_ENABLE_DIRECTX11 OR BOZA_ENABLE_DIRECTX12))
        message(FATAL_ERROR "At least one graphics backend must be enabled!")
    endif ()

    # Platform-specific backend validation
    if (NOT WIN32)
        foreach (backend DIRECTX11 DIRECTX12 XAUDIO2)
            if (BOZA_ENABLE_${backend})
                message(WARNING "${backend} is Windows-only, disabling...")
                set(BOZA_ENABLE_${backend} OFF CACHE BOOL "" FORCE)
            endif ()
        endforeach ()
    endif ()

    if (NOT APPLE)
        foreach (backend METAL COREAUDIO)
            if (BOZA_ENABLE_${backend})
                message(WARNING "${backend} is Apple-only, disabling...")
                set(BOZA_ENABLE_${backend} OFF CACHE BOOL "" FORCE)
            endif ()
        endforeach ()
    endif ()

    # Audio validation
    if (NOT (BOZA_ENABLE_OPENAL OR BOZA_ENABLE_XAUDIO2 OR BOZA_ENABLE_COREAUDIO))
        message(STATUS "No audio backend enabled")
        set(BOZA_AUDIO_ENABLED OFF PARENT_SCOPE)
    else ()
        set(BOZA_AUDIO_ENABLED ON PARENT_SCOPE)
    endif ()
endfunction()

_validate_backends()

# === Compile Definitions ===
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

# === Sanitizers ===
function(_apply_sanitizers)
    if (BOZA_ENABLE_ASAN AND BOZA_ENABLE_TSAN)
        message(FATAL_ERROR "Cannot enable both ASAN and TSAN simultaneously")
    endif ()

    if (MSVC)
        if (BOZA_ENABLE_ASAN AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.29")
            message(STATUS "AddressSanitizer enabled")
            add_compile_options(/fsanitize=address)
        elseif (BOZA_ENABLE_ASAN)
            message(WARNING "ASAN requires VS 2019 16.9+ on Windows")
        endif ()
    else ()
        set(sanitizers "")
        if (BOZA_ENABLE_ASAN)
            list(APPEND sanitizers "address")
        endif ()
        if (BOZA_ENABLE_UBSAN)
            list(APPEND sanitizers "undefined")
        endif ()
        if (BOZA_ENABLE_TSAN)
            list(APPEND sanitizers "thread")
        endif ()

        if (sanitizers)
            list(JOIN sanitizers "," sanitizer_flags)
            message(STATUS "Sanitizers enabled: ${sanitizer_flags}")
            add_compile_options(-fsanitize=${sanitizer_flags} -fno-omit-frame-pointer)
            add_link_options(-fsanitize=${sanitizer_flags})
        endif ()
    endif ()
endfunction()

_apply_sanitizers()

# === MSVC-Specific ===
if (MSVC)
    add_compile_options(
            /Zc:preprocessor
            /Zc:__cplusplus
            /Zc:externConstexpr
            /utf-8
    )
    add_compile_definitions(
            _CRT_SECURE_NO_WARNINGS
            _SCL_SECURE_NO_WARNINGS
    )
    add_compile_options(
            $<$<CONFIG:Release>:/O2>
            $<$<CONFIG:Release>:/GL>
            $<$<CONFIG:Release>:/Oi>
            $<$<CONFIG:Release>:/Gy>
    )
    add_link_options(
            $<$<CONFIG:Release>:/LTCG>
            $<$<CONFIG:Release>:/OPT:REF>
            $<$<CONFIG:Release>:/OPT:ICF>
    )
else ()
    add_compile_options(-fvisibility=hidden)
    add_compile_options(
            $<$<CONFIG:Debug>:-O1>

            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Release>:-march=native>
            $<$<CONFIG:Release>:-flto>
            $<$<CONFIG:Release>:-fomit-frame-pointer>
    )
    add_link_options(
            $<$<CONFIG:Release>:-flto>
            $<$<CONFIG:Release>:-Wl,--gc-sections>
    )
endif ()

# === Profiling ===
if (BOZA_ENABLE_PROFILING)
    add_compile_definitions(BOZA_PROFILING_ENABLED)
    if (NOT MSVC)
        add_compile_options(-pg)
        add_link_options(-pg)
    endif ()
endif ()

# === Mimalloc Setup ===
if (BOZA_USE_MIMALLOC)
    find_package(mimalloc CONFIG REQUIRED)

    add_library(mimalloc_runtime OBJECT)
    target_link_libraries(mimalloc_runtime PUBLIC mimalloc)

    set(BOZA_MIMALLOC_GLOBAL_CPP "${CMAKE_CURRENT_BINARY_DIR}/boza_mimalloc_global.cpp")
    file(WRITE "${BOZA_MIMALLOC_GLOBAL_CPP}" "#include <mimalloc-new-delete.h>\n")

    target_sources(mimalloc_runtime PRIVATE "${BOZA_MIMALLOC_GLOBAL_CPP}")

    message(STATUS "Mimalloc enabled (static linking)")
endif ()