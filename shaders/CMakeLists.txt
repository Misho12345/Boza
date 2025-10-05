file(GLOB_RECURSE SHADER_FILES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.geom"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.tesc"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.tese"
)

if (SHADER_FILES)
    set(GENERATED_FILES "")

    set(SHADER_FORMAT_ARGS "")

    if (NOT BOZA_ENABLE_OPENGL)
        list(APPEND SHADER_FORMAT_ARGS "--no-opengl")
    endif()

    if (NOT BOZA_ENABLE_DIRECTX11 AND NOT BOZA_ENABLE_DIRECTX12)
        list(APPEND SHADER_FORMAT_ARGS "--no-directx")
    endif()

    if (NOT BOZA_ENABLE_METAL)
        list(APPEND SHADER_FORMAT_ARGS "--no-metal")
    endif()


    foreach (SHADER_PATH ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)
        get_filename_component(SHADER_NAME_WE ${SHADER_PATH} NAME_WE)
        get_filename_component(SHADER_EXT ${SHADER_PATH} EXT)

        set(OUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${SHADER_NAME})
        set(SHADER_OUTPUTS "")

        set(SPV_OUT ${OUT_DIR}/${SHADER_NAME_WE}.spv)
        list(APPEND SHADER_OUTPUTS ${SPV_OUT})

        set(META_OUT ${OUT_DIR}/${SHADER_NAME_WE}.meta.json)
        list(APPEND SHADER_OUTPUTS ${META_OUT})


        if (BOZA_ENABLE_OPENGL)
            set(GLSL_OUT ${OUT_DIR}/${SHADER_NAME_WE}.glsl)
            list(APPEND SHADER_OUTPUTS ${GLSL_OUT})
        endif()

        if (BOZA_ENABLE_DIRECTX11 OR BOZA_ENABLE_DIRECTX12)
            set(HLSL_OUT ${OUT_DIR}/${SHADER_NAME_WE}.hlsl)
            list(APPEND SHADER_OUTPUTS ${HLSL_OUT})
        endif()

        if (BOZA_ENABLE_METAL)
            set(MSL_OUT ${OUT_DIR}/${SHADER_NAME_WE}.msl)
            list(APPEND SHADER_OUTPUTS ${MSL_OUT})
        endif()


        list(APPEND GENERATED_FILES ${SHADER_OUTPUTS})

        add_custom_command(
                OUTPUT ${SHADER_OUTPUTS}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
                COMMAND $<TARGET_FILE:shader_processor>
                ${SHADER_PATH}
                --out ${OUT_DIR}
                ${SHADER_FORMAT_ARGS}
                DEPENDS shader_processor ${SHADER_PATH}
                COMMENT "Compiling shader ${SHADER_NAME})"
                VERBATIM
        )
    endforeach()

    add_custom_target(compile_shaders ALL DEPENDS ${GENERATED_FILES})
else()
    message(STATUS "No shaders found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()
