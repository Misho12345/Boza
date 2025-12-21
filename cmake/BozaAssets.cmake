include_guard(GLOBAL)

# Create symlink/junction for asset directories
function(boza_link_assets target)
    set(asset_output_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets)
    set(asset_source_dir ${CMAKE_SOURCE_DIR}/assets)

    if (EXISTS ${asset_output_dir} OR NOT EXISTS ${asset_source_dir})
        return()
    endif ()

    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${asset_source_dir}
            ${asset_output_dir}
            COMMENT "Linking assets directory"
            VERBATIM
    )
endfunction()