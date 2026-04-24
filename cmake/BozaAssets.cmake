include_guard(GLOBAL)

function(boza_link_assets target)
    set(asset_source_dir "${CMAKE_SOURCE_DIR}/assets")
    set(shader_source_dir "${CMAKE_BINARY_DIR}/shaders")

    if (WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND powershell.exe -NoProfile -NonInteractive -Command
                "if (-not (Test-Path -LiteralPath '$<TARGET_FILE_DIR:${target}>/assets'))  { [void](New-Item -ItemType Junction -Path '$<TARGET_FILE_DIR:${target}>/assets'  -Value '${asset_source_dir}') };
                 if (-not (Test-Path -LiteralPath '$<TARGET_FILE_DIR:${target}>/shaders')) { [void](New-Item -ItemType Junction -Path '$<TARGET_FILE_DIR:${target}>/shaders' -Value '${shader_source_dir}') }"
                COMMENT "Linking assets and shaders (Windows junctions)"
                VERBATIM
        )
    else()
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${target}>"
                COMMAND bash -c "test -e '$<TARGET_FILE_DIR:${target}>/assets' || ln -s '${asset_source_dir}' '$<TARGET_FILE_DIR:${target}>/assets'"
                COMMAND bash -c "test -e '$<TARGET_FILE_DIR:${target}>/shaders' || ln -s '${shader_source_dir}' '$<TARGET_FILE_DIR:${target}>/shaders'"
                COMMENT "Linking assets and shaders (symlinks)"
                VERBATIM
        )
    endif()
endfunction()