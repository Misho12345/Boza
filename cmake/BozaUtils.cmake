include_guard(GLOBAL)

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

# snake_case to PascalCase conversion
function(boza_snake_to_pascal out_var in_text)
    string(REPLACE "_" ";" word_list "${in_text}")

    set(result "")
    foreach (word IN LISTS word_list)
        if (word)
            string(SUBSTRING "${word}" 0 1 first_char)
            string(TOUPPER "${first_char}" first_char)
            string(LENGTH "${word}" word_len)
            if (word_len GREATER 1)
                string(SUBSTRING "${word}" 1 -1 rest)
                string(APPEND result "${first_char}${rest}")
            else ()
                string(APPEND result "${first_char}")
            endif ()
        endif ()
    endforeach ()

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()