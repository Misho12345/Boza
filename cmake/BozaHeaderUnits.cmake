include_guard(GLOBAL)

set(HEADER_UNITS_DIR "${CMAKE_BINARY_DIR}/header_units")
file(MAKE_DIRECTORY "${HEADER_UNITS_DIR}")

# === Helper Functions ===
function(_make_name_safe out_var input_path)
    string(REPLACE "/" "_" tmp "${input_path}")
    string(REPLACE "\\" "_" result "${tmp}")
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(_build_compile_flags out_var definitions include_dirs)
    set(flags "")

    # Add definitions
    foreach (def IN LISTS definitions)
        if (MSVC)
            list(APPEND flags "/D${def}")
        else ()
            list(APPEND flags "-D${def}")
        endif ()
    endforeach ()

    # Add include directories
    foreach (inc_dir IN LISTS include_dirs)
        if (MSVC)
            list(APPEND flags "/external:I${inc_dir}")
        else ()
            list(APPEND flags "-I${inc_dir}")
        endif ()
    endforeach ()

    set(${out_var} ${flags} PARENT_SCOPE)
endfunction()

# === Create Header Unit Library ===
function(add_header_unit_library library_name)
    if (TARGET ${library_name})
        return()
    endif ()

    add_custom_target(${library_name})
    set_target_properties(${library_name} PROPERTIES
            BOZA_HEADER_UNIT_LIBRARY TRUE
            BOZA_HEADER_UNITS ""
    )

    file(MAKE_DIRECTORY "${HEADER_UNITS_DIR}/${library_name}")
endfunction()

# === Add Header Unit to Library ===
function(target_add_header_unit library_name header_include_dir header_rel_path)
    cmake_parse_arguments(ARG "" "" "COMPILE_DEFINITIONS;INCLUDE_DIRECTORIES" ${ARGN})

    # Validate target
    if (NOT TARGET ${library_name})
        message(FATAL_ERROR "target_add_header_unit: '${library_name}' is not a target. Call add_header_unit_library() first.")
    endif ()

    get_target_property(is_hu_lib ${library_name} BOZA_HEADER_UNIT_LIBRARY)
    if (NOT is_hu_lib)
        message(FATAL_ERROR "target_add_header_unit: '${library_name}' is not a header unit library.")
    endif ()

    # Setup library-specific paths
    set(header_abs_path "${header_include_dir}/${header_rel_path}")
    set(library_output_dir "${HEADER_UNITS_DIR}/${library_name}")

    get_filename_component(hu_subdir "${header_rel_path}" DIRECTORY)
    if (hu_subdir)
        file(MAKE_DIRECTORY "${library_output_dir}/${hu_subdir}")
    endif ()

    # Create unique target name per library
    _make_name_safe(target_suffix "${header_rel_path}")
    set(target_name "header_unit_${library_name}_${target_suffix}")

    # Build compiler flags
    _build_compile_flags(compile_flags "${ARG_COMPILE_DEFINITIONS}" "${ARG_INCLUDE_DIRECTORIES}")

    # Compiler-specific compilation
    if (MSVC)
        set(out_file "${library_output_dir}/${header_rel_path}.ifc")
        set(obj_file "${library_output_dir}/${header_rel_path}.obj")
        set(runtime_flag "$<IF:$<CONFIG:Debug>,/MDd,/MD>")

        add_custom_command(
                OUTPUT "${out_file}" "${obj_file}"
                COMMAND ${CMAKE_CXX_COMPILER}
                /nologo /c /EHsc /std:c++latest ${runtime_flag}
                "/external:I${header_include_dir}" /external:W0
                ${compile_flags}
                /ifcOutput "${out_file}"
                "/Fo${obj_file}"
                /exportHeader "${header_abs_path}"
                DEPENDS "${header_abs_path}"
                COMMENT "Building header unit for ${library_name}: ${header_rel_path}"
                VERBATIM
        )

        set_source_files_properties("${out_file}" "${obj_file}" PROPERTIES GENERATED TRUE)

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Clang header unit support is not implemented.")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "GCC header unit support is not implemented.")
    endif ()

    add_custom_target(${target_name} ALL DEPENDS "${out_file}")

    if (TARGET boza_vcpkg_header_fixes)
        add_dependencies(${target_name} boza_vcpkg_header_fixes)
    endif ()

    # Store header unit info (now including library_name for path resolution)
    get_target_property(hu_list ${library_name} BOZA_HEADER_UNITS)
    if (NOT hu_list OR hu_list STREQUAL "hu_list-NOTFOUND")
        set(hu_list "")
    endif ()

    string(REPLACE ";" "," defs_str "${ARG_COMPILE_DEFINITIONS}")
    list(APPEND hu_list "${header_include_dir}|${header_rel_path}|${target_name}|${defs_str}")
    set_target_properties(${library_name} PROPERTIES BOZA_HEADER_UNITS "${hu_list}")
endfunction()

# === Enable Header Unit for Target ===
function(_enable_header_unit target library_name header_include_dir header_rel_path header_unit_target definitions)
    set(library_output_dir "${HEADER_UNITS_DIR}/${library_name}")

    if (MSVC)
        set(ifc_file "${library_output_dir}/${header_rel_path}.ifc")
        set(obj_file "${library_output_dir}/${header_rel_path}.obj")
        file(TO_CMAKE_PATH "${header_include_dir}/${header_rel_path}" header_path)
        file(TO_CMAKE_PATH "${ifc_file}" ifc_path)
        target_compile_options(${target} PRIVATE "/headerUnit${header_path}=${ifc_path}")
        target_sources(${target} PRIVATE "${obj_file}")

    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "Clang header unit enabling is not implemented.")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "GCC header unit enabling is not implemented.")
    endif ()

    if (definitions)
        string(REPLACE "," ";" defs_list "${definitions}")
        target_compile_definitions(${target} PRIVATE ${defs_list})
    endif ()

    add_dependencies(${target} ${header_unit_target})
endfunction()

# === Link Header Units to Target ===
function(target_link_header_units target library_name)
    # Validate targets
    if (NOT TARGET ${target})
        message(FATAL_ERROR "target_link_header_units: '${target}' is not a valid target.")
    endif ()

    if (NOT TARGET ${library_name})
        message(FATAL_ERROR "target_link_header_units: '${library_name}' is not a valid target.")
    endif ()

    get_target_property(is_hu_lib ${library_name} BOZA_HEADER_UNIT_LIBRARY)
    if (NOT is_hu_lib)
        message(FATAL_ERROR "target_link_header_units: '${library_name}' is not a header unit library.")
    endif ()

    # Get header unit list
    get_target_property(hu_list ${library_name} BOZA_HEADER_UNITS)
    if (NOT hu_list OR hu_list STREQUAL "hu_list-NOTFOUND")
        message(WARNING "target_link_header_units: '${library_name}' has no header units.")
        return()
    endif ()

    # Enable each header unit
    foreach (entry IN LISTS hu_list)
        string(REPLACE "|" ";" parts "${entry}")
        list(GET parts 0 header_include_dir)
        list(GET parts 1 header_rel_path)
        list(GET parts 2 header_unit_target)
        list(GET parts 3 defs_str)

        _enable_header_unit(${target}
                "${library_name}"
                "${header_include_dir}"
                "${header_rel_path}"
                "${header_unit_target}"
                "${defs_str}"
        )
    endforeach ()
endfunction()