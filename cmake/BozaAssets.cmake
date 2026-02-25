include_guard(GLOBAL)

# Create symlink/junction for asset directory
function(boza_link_assets target)
    set(asset_output_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets")
    set(asset_source_dir "${CMAKE_SOURCE_DIR}/assets")

    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            COMMENT "Ensuring runtime output directory exists"
            VERBATIM
    )

    if (WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND powershell.exe -NoProfile -NonInteractive -Command
                "if (-not (Test-Path -LiteralPath '${asset_output_dir}')) { [void](New-Item -ItemType Junction -Path '${asset_output_dir}' -Value '${asset_source_dir}') }"
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