# Function to create a symlink/junction for asset directories
function(boza_link_assets TARGET_NAME)
    set(ASSET_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets)

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${ASSET_OUTPUT_DIR}
        COMMENT "Creating assets directory"
    )

    if(EXISTS ${CMAKE_SOURCE_DIR}/textures)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                ${CMAKE_SOURCE_DIR}/textures
                ${ASSET_OUTPUT_DIR}/textures
            COMMENT "Linking textures directory"
            VERBATIM
        )
    endif()
endfunction()

function(boza_define_asset_paths TARGET_NAME)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        BOZA_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
        BOZA_RUNTIME_DIR="$<TARGET_FILE_DIR:${TARGET_NAME}>"
        BOZA_ASSETS_DIR="$<TARGET_FILE_DIR:${TARGET_NAME}>/assets"
        BOZA_SHADERS_DIR="$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders"
    )
endfunction()

