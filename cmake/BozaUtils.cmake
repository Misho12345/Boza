# Gets all source files from the specified directories and their subdirectories.
function(get_source_files out_sources)
    if (NOT out_sources)
        message(FATAL_ERROR "Output variable name must be specified")
    endif ()

    list(LENGTH ARGN arg_count)
    if (arg_count EQUAL 0)
        message(FATAL_ERROR "At least base directory must be specified")
    endif ()

    set(collected_sources_list "")

    foreach (arg IN LISTS ARGN)
        if (IS_DIRECTORY "${arg}")
            file(GLOB_RECURSE dir_sources
                    CONFIGURE_DEPENDS
                    "${arg}/*.hpp"
                    "${arg}/*.hh"
                    "${arg}/*.h"
                    "${arg}/*.inl"
                    "${arg}/*.ipp"
                    "${arg}/*.cpp"
                    "${arg}/*.cc"
                    "${arg}/*.cxx"
                    "${arg}/*.c"
            )

            list(APPEND collected_sources_list ${dir_sources})
        else ()
            message(WARNING "Skipping '${arg}': Not a valid directory")
        endif ()
    endforeach ()

    set(${out_sources} ${collected_sources_list} PARENT_SCOPE)
endfunction()

# Sets up include directories for a target with a standard structure for the engine.
function(set_default_include_dirs target dir)
    if (NOT target)
        message(FATAL_ERROR "Target name must be specified")
    endif ()

    if (NOT dir)
        message(FATAL_ERROR "Directory must be specified")
    endif ()

    target_include_directories(${target}
            PUBLIC
            $<BUILD_INTERFACE:${dir}/include>
            $<INSTALL_INTERFACE:include>
            PRIVATE
            $<BUILD_INTERFACE:${dir}/src>
    )
endfunction()

# Creates a test executable target with GoogleTest and links it to the tested target.
function(create_tests test_target tested_target test_source_dir)
    if (NOT test_target)
        message(FATAL_ERROR "Test target name must be specified")
    endif ()

    if (NOT tested_target)
        message(FATAL_ERROR "Tested target name must be specified")
    endif ()

    if (NOT test_source_dir)
        message(FATAL_ERROR "Test source directory must be specified")
    endif ()

    find_package(GTest CONFIG REQUIRED)

    get_source_files(${test_target}_SOURCES ${test_source_dir})
    add_executable(${test_target} ${${test_target}_SOURCES})

    target_link_libraries(${test_target}
            PRIVATE
            ${tested_target}
            GTest::gmock_main
    )

    boza_enable_warnings(${test_target})

    include(GoogleTest)
    gtest_discover_tests(${test_target} DISCOVERY_TIMEOUT 60)
endfunction()

# for RHI and AHI backends
function(setup_backend backend_name interface_target final_target source_dir)
    if (NOT backend_name)
        message(FATAL_ERROR "Backend name is required")
    endif ()

    if (NOT interface_target)
        message(FATAL_ERROR "Interface target name is required")
    endif ()

    if (NOT final_target)
        message(FATAL_ERROR "Final target name is required")
    endif ()

    if (NOT source_dir OR NOT IS_DIRECTORY ${source_dir})
        message(FATAL_ERROR "Source directory is required to be a valid directory")
    endif ()

    add_library(${backend_name} STATIC)

    get_source_files(${backend_name}_SOURCES ${source_dir})
    target_sources(${backend_name} PRIVATE ${${backend_name}_SOURCES})
    target_include_directories(${backend_name} PRIVATE ${source_dir})

    target_link_libraries(${backend_name} PRIVATE ${interface_target})
    target_precompile_headers(${backend_name} PRIVATE ${source_dir}/pch.hpp)

    boza_enable_warnings(${backend_name})

    target_link_libraries(${final_target} PUBLIC ${backend_name})
endfunction()