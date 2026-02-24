include_guard(GLOBAL)

# Create symlink/junction for asset directory
function(boza_link_assets target)
    set(asset_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")
    set(asset_source_dir "${CMAKE_SOURCE_DIR}/assets")

    if (EXISTS "${asset_output_dir}")
        return()
    endif ()

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMENT "Linking assets directory"
        VERBATIM
    )

    if (WIN32)
        file(TO_NATIVE_PATH "${asset_output_dir}" asset_output_native)
        file(TO_NATIVE_PATH "${asset_source_dir}" asset_source_native)

        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            COMMAND cmd.exe /V:OFF /C mklink /J "${asset_output_native}" "${asset_source_native}"
            COMMENT "Linking assets directory (Windows junction)"
            VERBATIM
        )
    else()
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E create_symlink "${asset_source_dir}" "${asset_output_dir}"
            COMMENT "Linking assets directory (symlink)"
            VERBATIM
        )
    endif()
endfunction()
