set(HEADER_UNITS_DIR "${CMAKE_BINARY_DIR}/header_units")
file(MAKE_DIRECTORY "${HEADER_UNITS_DIR}")

function(make_name_safe out_var input_path)
    string(REPLACE "/" "_" tmp "${input_path}")
    string(REPLACE "\\" "_" tmp2 "${tmp}")
    set(${out_var} "${tmp2}" PARENT_SCOPE)
endfunction()

function(create_header_unit header_include_dir header_rel_path)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs COMPILE_DEFINITIONS INCLUDE_DIRECTORIES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(header_abs_path "${header_include_dir}/${header_rel_path}")

    get_filename_component(_hu_subdir "${header_rel_path}" DIRECTORY)
    if (_hu_subdir)
        file(MAKE_DIRECTORY "${HEADER_UNITS_DIR}/${_hu_subdir}")
    endif()

    make_name_safe(target_suffix "${header_rel_path}")
    set(target_name "header_unit_${target_suffix}")

    set(compile_defs_flags "")
    if (ARG_COMPILE_DEFINITIONS)
        foreach(def IN LISTS ARG_COMPILE_DEFINITIONS)
            if (MSVC)
                list(APPEND compile_defs_flags "/D${def}")
            else()
                list(APPEND compile_defs_flags "-D${def}")
            endif()
        endforeach()
    endif()

    set(include_flags "")
    if (ARG_INCLUDE_DIRECTORIES)
        foreach(inc_dir IN LISTS ARG_INCLUDE_DIRECTORIES)
            if (MSVC)
                list(APPEND include_flags "/external:I${inc_dir}")
            else()
                list(APPEND include_flags "-I${inc_dir}")
            endif()
        endforeach()
    endif()

    if (MSVC)
        set(ifc_file "${HEADER_UNITS_DIR}/${header_rel_path}.ifc")
        set(obj_file "${HEADER_UNITS_DIR}/${header_rel_path}.obj")

        if (CMAKE_BUILD_TYPE STREQUAL "Release")
            set(RUNTIME_FLAG "/MD")
        else ()
            set(RUNTIME_FLAG "/MDd")
        endif ()

        add_custom_command(
                OUTPUT "${ifc_file}"
                COMMAND ${CMAKE_CXX_COMPILER}
                /nologo /c /EHsc /std:c++latest
                ${RUNTIME_FLAG}
                "/external:I${header_include_dir}"
                /external:W0
                ${include_flags}
                ${compile_defs_flags}
                /ifcOutput "${ifc_file}"
                "/Fo${obj_file}"
                /exportHeader "${header_abs_path}"
                DEPENDS "${header_abs_path}"
                VERBATIM
        )

        set(out_file "${ifc_file}")

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(pcm_file "${HEADER_UNITS_DIR}/${header_rel_path}.pcm")

        add_custom_command(
                OUTPUT "${pcm_file}"
                COMMAND ${CMAKE_CXX_COMPILER}
                -std=c++23
                -fmodule-header=user
                -xc++-header
                ${include_flags}
                ${compile_defs_flags}
                "${header_abs_path}"
                -o "${pcm_file}"
                DEPENDS "${header_abs_path}"
                VERBATIM
        )

        set(out_file "${pcm_file}")

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(gcm_file "${HEADER_UNITS_DIR}/${header_rel_path}.gcm")

        add_custom_command(
                OUTPUT "${gcm_file}"
                COMMAND ${CMAKE_CXX_COMPILER}
                -std=c++23
                -x c++-header
                -fmodules-ts
                ${include_flags}
                ${compile_defs_flags}
                "${header_abs_path}"
                -o "${gcm_file}"
                DEPENDS "${header_abs_path}"
                VERBATIM
        )

        set(out_file "${gcm_file}")
    endif ()

    add_custom_target(${target_name} ALL DEPENDS "${out_file}")

    get_property(_boza_header_units GLOBAL PROPERTY BOZA_HEADER_UNITS)
    if (NOT _boza_header_units OR _boza_header_units STREQUAL "_boza_header_units-NOTFOUND")
        set(_boza_header_units "")
    endif ()

    # Format: header_include_dir|header_rel_path|def1;def2;def3
    set(defs_str "")
    if (ARG_COMPILE_DEFINITIONS)
        string(REPLACE ";" "," defs_str "${ARG_COMPILE_DEFINITIONS}")
    endif()

    list(APPEND _boza_header_units "${header_include_dir}|${header_rel_path}|${defs_str}")
    set_property(GLOBAL PROPERTY BOZA_HEADER_UNITS "${_boza_header_units}")
endfunction()

function(import_header_unit target header_include_dir header_rel_path)
    set(options APPLY_DEFINITIONS)
    set(oneValueArgs "")
    set(multiValueArgs COMPILE_DEFINITIONS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(HEADER_UNITS_DIR "${CMAKE_BINARY_DIR}/header_units")

    if (MSVC)
        set(ifc_file "${HEADER_UNITS_DIR}/${header_rel_path}.ifc")

        file(TO_CMAKE_PATH "${header_include_dir}/${header_rel_path}" header_unit_header)
        file(TO_CMAKE_PATH "${ifc_file}" header_unit_ifc)

        set(mapping "${header_unit_header}=${header_unit_ifc}")
        target_compile_options(${target} PRIVATE "/headerUnit${mapping}")

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(pcm_file "${HEADER_UNITS_DIR}/${header_rel_path}.pcm")

        target_compile_options(${target} PRIVATE
                -fmodule-file=${pcm_file}
        )

        set_property(TARGET ${target} APPEND PROPERTY
                CXX_SCANDEP_FLAGS
                -fmodule-file=${pcm_file}
        )

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(gcm_file "${HEADER_UNITS_DIR}/${header_rel_path}.gcm")
        target_compile_options(${target} PRIVATE
                -fmodule-header=${header_rel_path}=${gcm_file}
        )
    endif ()

    if (ARG_APPLY_DEFINITIONS AND ARG_COMPILE_DEFINITIONS)
        target_compile_definitions(${target} PRIVATE ${ARG_COMPILE_DEFINITIONS})
    endif()

    make_name_safe(target_suffix "${header_rel_path}")
    add_dependencies(${target} header_unit_${target_suffix})
endfunction()


function(import_header_units target)
    set(options APPLY_DEFINITIONS)
    set(oneValueArgs "")
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_property(_boza_header_units GLOBAL PROPERTY BOZA_HEADER_UNITS)

    if (NOT _boza_header_units OR _boza_header_units STREQUAL "_boza_header_units-NOTFOUND")
        message(FATAL_ERROR
                "import_header_units(${target}): no header units registered. "
                "Call create_header_unit(...) before import_header_units()."
        )
    endif ()

    foreach(_entry IN LISTS _boza_header_units)
        string(REPLACE "|" ";" _parts "${_entry}")
        list(GET _parts 0 _header_include_dir)
        list(GET _parts 1 _header_rel_path)
        list(GET _parts 2 _defs_str)

        set(_compile_defs "")
        if (_defs_str)
            string(REPLACE "," ";" _compile_defs "${_defs_str}")
        endif()

        if (ARG_APPLY_DEFINITIONS AND _compile_defs)
            import_header_unit(${target} "${_header_include_dir}" "${_header_rel_path}"
                    APPLY_DEFINITIONS COMPILE_DEFINITIONS ${_compile_defs})
        else()
            import_header_unit(${target} "${_header_include_dir}" "${_header_rel_path}")
        endif()
    endforeach()
endfunction()