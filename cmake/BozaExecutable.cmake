include_guard(GLOBAL)

function(boza_add_executable target)
    add_executable(${target})
    boza_enable_warnings(${target})

    # === Collect Sources/Modules ===
    boza_get_modules(all_modules ${CMAKE_CURRENT_SOURCE_DIR})
    file(GLOB_RECURSE all_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

    # === Auto-generate main TU ===
    boza_snake_to_pascal(app_class "${target}")

    boza_generate_impl(${target}
            "import ${target};

            int main()
            {
                ${app_class} app;
                if (!app.init()) return -1;
                app.run();
            }"
            "main.cpp")


    # === Setup Module Target ===
    if (all_modules OR all_sources)
        boza_setup_module_target(${target} ${CMAKE_CURRENT_SOURCE_DIR}
                PRIVATE_MODULES ${all_modules}
                SOURCES ${all_sources}
        )
    endif ()

    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    if (BOZA_USE_MIMALLOC AND TARGET mimalloc_runtime)
        target_link_libraries(${target} PRIVATE mimalloc_runtime)
    endif ()

    target_link_libraries(${target} PRIVATE Boza::Engine)

    target_link_header_units(${target} boza_engine_header_units)
    add_dependencies(${target} compile_shaders)

    boza_link_assets(${target})
endfunction()
