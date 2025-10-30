# Function to create a symlink/junction for asset directories
function(boza_link_assets TARGET_NAME)
    if (CMAKE_CONFIGURATION_TYPES)
        # Multi-config generator (Visual Studio, Xcode)
        # Use generator expression to only run for the actual build config
        set(ASSET_OUTPUT_DIR $<TARGET_FILE_DIR:${TARGET_NAME}>/assets)

        if (EXISTS ${CMAKE_SOURCE_DIR}/assets)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${TARGET_NAME}>
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                    ${CMAKE_SOURCE_DIR}/assets
                    ${ASSET_OUTPUT_DIR}
                    COMMENT "Linking assets directory for $<CONFIG>"
                    VERBATIM
            )
        endif ()
    else ()
        # Single-config generator (Unix Makefiles, Ninja)
        set(ASSET_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets)

        if (NOT EXISTS ${ASSET_OUTPUT_DIR} AND EXISTS ${CMAKE_SOURCE_DIR}/assets)
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                    ${CMAKE_SOURCE_DIR}/assets
                    ${ASSET_OUTPUT_DIR}
                    COMMENT "Linking assets directory"
                    VERBATIM
            )
        endif ()
    endif ()
endfunction()