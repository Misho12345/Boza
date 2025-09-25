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

    foreach (SHADER_PATH ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)
        get_filename_component(SHADER_NAME_WE ${SHADER_PATH} NAME_WE)

        set(OUT_DIR ${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME})

        set(SPV_OUT ${OUT_DIR}/${SHADER_NAME_WE}.spv)
        set(GLSL_OUT ${OUT_DIR}/${SHADER_NAME_WE}.glsl)
        set(HLSL_OUT ${OUT_DIR}/${SHADER_NAME_WE}.hlsl)
        set(MSL_OUT ${OUT_DIR}/${SHADER_NAME_WE}.msl)
        set(META_OUT ${OUT_DIR}/${SHADER_NAME_WE}.meta.json)

        list(APPEND GENERATED_FILES ${SPV_OUT} ${GLSL_OUT} ${HLSL_OUT} ${MSL_OUT} ${META_OUT})

        add_custom_command(
                OUTPUT ${SPV_OUT} ${GLSL_OUT} ${HLSL_OUT} ${MSL_OUT} ${META_OUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}
                COMMAND $<TARGET_FILE:shader_processor> ${SHADER_PATH} --out ${OUT_DIR}
                DEPENDS shader_processor ${SHADER_PATH}
                COMMENT "Compiling shader ${SHADER_NAME} -> ${OUT_DIR}"
                VERBATIM
        )
    endforeach ()

    add_custom_target(compile_shaders ALL DEPENDS ${GENERATED_FILES})
else ()
    message(STATUS "No shaders found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif ()
