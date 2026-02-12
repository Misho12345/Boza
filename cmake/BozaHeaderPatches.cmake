include_guard(GLOBAL)

function(boza_enable_vcpkg_header_fixes)
    if (TARGET boza_vcpkg_header_fixes)
        return()
    endif ()

    if (NOT DEFINED VCPKG_TARGET_TRIPLET OR VCPKG_TARGET_TRIPLET STREQUAL "")
        message(FATAL_ERROR "VCPKG_TARGET_TRIPLET is not defined")
    endif ()

    if (DEFINED VCPKG_INSTALLED_DIR AND NOT VCPKG_INSTALLED_DIR STREQUAL "")
        set(_vcpkg_root "${VCPKG_INSTALLED_DIR}")
    else ()
        set(_vcpkg_root "${CMAKE_BINARY_DIR}/vcpkg_installed")
    endif ()

    set(_inc "${_vcpkg_root}/${VCPKG_TARGET_TRIPLET}/include")

    set(_flecs_files
            "${_inc}/flecs/addons/cpp/mixins/pipeline/decl.hpp"
            "${_inc}/flecs/addons/cpp/c_types.hpp"
            "${_inc}/flecs/addons/cpp/mixins/meta/decl.hpp"
    )

    set(_gtl_file "${_inc}/gtl/gtl_base.hpp")

    set(_script_dir "${CMAKE_BINARY_DIR}/cmake")
    set(_script "${_script_dir}/boza_vcpkg_header_fixes_impl.cmake")
    set(_stamp "${CMAKE_BINARY_DIR}/boza_vcpkg_header_fixes.stamp")
    file(MAKE_DIRECTORY "${_script_dir}")

    file(WRITE "${_script}" [[
if(NOT DEFINED FLECS_FILES)
  message(FATAL_ERROR "FLECS_FILES not set")
endif()

if(NOT DEFINED GTL_FILE)
  message(FATAL_ERROR "GTL_FILE not set")
endif()

if(NOT DEFINED STAMP_FILE)
  message(FATAL_ERROR "STAMP_FILE not set")
endif()

function(_boza_replace_in_file path from to)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "Missing file: ${path}")
  endif()
  file(READ "${path}" _txt)
  string(REPLACE "${from}" "${to}" _txt2 "${_txt}")
  if(NOT _txt STREQUAL _txt2)
    file(WRITE "${path}" "${_txt2}")
  endif()
endfunction()

foreach(f IN LISTS FLECS_FILES)
  _boza_replace_in_file("${f}" "static " "")
endforeach()

if(NOT EXISTS "${GTL_FILE}")
  message(FATAL_ERROR "Missing file: ${GTL_FILE}")
endif()

file(READ "${GTL_FILE}" gtl_txt)
string(REPLACE "static inline void ThrowStdOutOfRange" "inline void ThrowStdOutOfRange" gtl_txt2 "${gtl_txt}")
if(NOT gtl_txt STREQUAL gtl_txt2)
  file(WRITE "${GTL_FILE}" "${gtl_txt2}")
endif()

file(TOUCH "${STAMP_FILE}")
]])

    list(JOIN _flecs_files ";" _flecs_files_arg)

    add_custom_command(
            OUTPUT "${_stamp}"
            COMMAND "${CMAKE_COMMAND}"
            "-DFLECS_FILES=${_flecs_files_arg}"
            "-DGTL_FILE=${_gtl_file}"
            "-DSTAMP_FILE=${_stamp}"
            -P "${_script}"
            DEPENDS
            "${_script}"
            ${_flecs_files}
            "${_gtl_file}"
            VERBATIM
            COMMENT "Boza: patching vcpkg headers"
    )

    add_custom_target(boza_vcpkg_header_fixes DEPENDS "${_stamp}")
endfunction()
