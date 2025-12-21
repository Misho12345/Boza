include_guard(GLOBAL)

# Create an executable that depends on Boza Engine with standard setup
function(boza_add_executable target)
    cmake_parse_arguments(ARG
            ""
            "SOURCE_DIR"
            "SOURCES;MODULES;LINK_LIBRARIES;COMPILE_DEFINITIONS;INCLUDE_DIRECTORIES"
            ${ARGN}
    )

    # Default to current source directory
    if (NOT ARG_SOURCE_DIR)
        set(ARG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif ()

    # === Create Executable ===
    add_executable(${target})
    boza_enable_warnings(${target})

    # === Collect Sources/Modules ===
    set(all_modules ${ARG_MODULES})
    set(all_sources ${ARG_SOURCES})

    # Auto-collect if not explicitly provided
    if (NOT all_modules)
        boza_get_modules(all_modules ${ARG_SOURCE_DIR})
    endif ()

    if (NOT all_sources)
        file(GLOB_RECURSE all_sources CONFIGURE_DEPENDS
                "${ARG_SOURCE_DIR}/*.cpp"
        )
    endif ()

    # === Setup Module Target ===
    if (all_modules OR all_sources)
        boza_setup_module_target(${target} ${ARG_SOURCE_DIR}
                PRIVATE_MODULES ${all_modules}
                SOURCES ${all_sources}
        )
    endif ()

    # === Additional Configuration ===
    if (ARG_INCLUDE_DIRECTORIES)
        target_include_directories(${target} PRIVATE ${ARG_INCLUDE_DIRECTORIES})
    endif ()

    if (ARG_COMPILE_DEFINITIONS)
        target_compile_definitions(${target} PRIVATE ${ARG_COMPILE_DEFINITIONS})
    endif ()

    # === Link Dependencies ===
    target_link_libraries(${target} PRIVATE Boza::Engine)

    if (ARG_LINK_LIBRARIES)
        target_link_libraries(${target} PRIVATE ${ARG_LINK_LIBRARIES})
    endif ()

    # === Standard Setup ===
    if (BOZA_USE_MIMALLOC AND TARGET mimalloc_runtime)
        target_link_libraries(${target} PRIVATE mimalloc_runtime)
    endif ()

    if (TARGET boza_engine_header_units)
        target_link_header_units(${target} boza_engine_header_units)
    endif ()

    if (TARGET compile_shaders)
        add_dependencies(${target} compile_shaders)
    endif ()

    boza_link_assets(${target})
endfunction()