file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.geom"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.tesc"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.tese"
)

if (NOT SHADER_FILES)
    message(STATUS "No shaders found in ${CMAKE_CURRENT_SOURCE_DIR}")
    return()
endif ()

# === Build Format Arguments ===
set(format_args "")
if (NOT BOZA_ENABLE_OPENGL)
    list(APPEND format_args "--no-opengl")
endif ()
if (NOT (BOZA_ENABLE_DIRECTX11 OR BOZA_ENABLE_DIRECTX12))
    list(APPEND format_args "--no-directx")
endif ()
if (NOT BOZA_ENABLE_METAL)
    list(APPEND format_args "--no-metal")
endif ()

# === Compile Shaders ===
set(shader_outputs "")
set(shader_out_base ${CMAKE_BINARY_DIR}/shaders)

foreach (shader_path ${SHADER_FILES})
    get_filename_component(shader_name ${shader_path} NAME)
    get_filename_component(shader_name_we ${shader_path} NAME_WE)
    file(RELATIVE_PATH shader_rel ${CMAKE_CURRENT_SOURCE_DIR} ${shader_path})

    set(out_dir ${shader_out_base}/${shader_rel})
    set(outputs "")

    # Always generate SPIR-V and metadata
    list(APPEND outputs
            ${out_dir}/${shader_name_we}.spv
            ${out_dir}/${shader_name_we}.meta.json
    )

    # Backend-specific outputs
    if (BOZA_ENABLE_OPENGL)
        list(APPEND outputs ${out_dir}/${shader_name_we}.glsl)
    endif ()
    if (BOZA_ENABLE_DIRECTX11 OR BOZA_ENABLE_DIRECTX12)
        list(APPEND outputs ${out_dir}/${shader_name_we}.hlsl)
    endif ()
    if (BOZA_ENABLE_METAL)
        list(APPEND outputs ${out_dir}/${shader_name_we}.msl)
    endif ()

    list(APPEND shader_outputs ${outputs})

    add_custom_command(
            OUTPUT ${outputs}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${out_dir}
            COMMAND $<TARGET_FILE:Tools::ShaderProcessor>
            ${shader_path}
            --out ${out_dir}
            ${format_args}
            DEPENDS Tools::ShaderProcessor ${shader_path}
            COMMENT "Compiling shader ${shader_name}"
            VERBATIM
    )
endforeach ()

add_custom_target(compile_shaders ALL DEPENDS ${shader_outputs})
