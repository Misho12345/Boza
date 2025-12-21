include_guard(GLOBAL)

# Collect source files from directories
function(boza_get_sources out_var)
    set(sources "")
    foreach (dir IN LISTS ARGN)
        if (IS_DIRECTORY "${dir}")
            file(GLOB_RECURSE dir_sources CONFIGURE_DEPENDS
                    "${dir}/*.hpp" "${dir}/*.hh" "${dir}/*.h"
                    "${dir}/*.inl" "${dir}/*.ipp"
                    "${dir}/*.cpp" "${dir}/*.cc" "${dir}/*.cxx"
            )
            list(APPEND sources ${dir_sources})
        else ()
            message(WARNING "Skipping '${dir}': not a valid directory")
        endif ()
    endforeach ()
    set(${out_var} ${sources} PARENT_SCOPE)
endfunction()

# Collect module interface files
function(boza_get_modules out_var)
    set(modules "")
    foreach (dir IN LISTS ARGN)
        if (IS_DIRECTORY "${dir}")
            file(GLOB_RECURSE dir_modules CONFIGURE_DEPENDS
                    "${dir}/*.ixx" "${dir}/*.cppm"
            )
            list(APPEND modules ${dir_modules})
        endif ()
    endforeach ()
    set(${out_var} ${modules} PARENT_SCOPE)
endfunction()

# Generate implementation file for header-only libraries
function(boza_generate_impl target header_content out_file)
    set(impl_file "${CMAKE_CURRENT_BINARY_DIR}/${out_file}")
    file(WRITE "${impl_file}" "${header_content}")
    target_sources(${target} PRIVATE "${impl_file}")
endfunction()

# Setup module target with common configuration
function(boza_setup_module_target target base_dir)
    cmake_parse_arguments(ARG "" "" "PUBLIC_MODULES;PRIVATE_MODULES;SOURCES" ${ARGN})

    target_include_directories(${target} PRIVATE
            $<BUILD_INTERFACE:${base_dir}>
    )

    if (ARG_PUBLIC_MODULES)
        target_sources(${target}
                PUBLIC FILE_SET CXX_MODULES
                TYPE CXX_MODULES
                BASE_DIRS ${base_dir}
                FILES ${ARG_PUBLIC_MODULES}
        )
    endif ()

    if (ARG_PRIVATE_MODULES)
        target_sources(${target}
                PRIVATE FILE_SET private_modules
                TYPE CXX_MODULES
                BASE_DIRS ${base_dir}
                FILES ${ARG_PRIVATE_MODULES}
        )
    endif ()

    if (ARG_SOURCES)
        target_sources(${target} PRIVATE ${ARG_SOURCES})
    endif ()
endfunction()