function(get_source_files out_sources)
    if (NOT out_sources)
        message(FATAL_ERROR "Output variable name must be specified")
    endif ()

    list(LENGTH ARGN arg_count)
    if (arg_count EQUAL 0)
        message(FATAL_ERROR "At least base directory must be specified")
    endif ()

    set(_collected_sources_list "")

    foreach (arg IN LISTS ARGN)
        if(IS_DIRECTORY "${arg}")
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

            list(APPEND _collected_sources_list ${dir_sources})
        else()
            message(WARNING "Skipping '${arg}': Not a valid directory")
        endif()
    endforeach ()

    set(_filtered_sources "")
    foreach(_f IN LISTS _collected_sources_list)
        if (_f MATCHES "[/\\\\]build([/\\\\]|$)")
            message(VERBOSE "get_source_files: filtered out (in build dir): ${_f}")
            continue()
        endif()

        if (_f MATCHES "\\.gch$" OR _f MATCHES "\\.pch$" OR
                _f MATCHES "\\.o$" OR _f MATCHES "\\.obj$" OR
                _f MATCHES "\\.a$" OR _f MATCHES "\\.so$" OR _f MATCHES "\\.dll$" OR
                _f MATCHES "\\.lib$" OR _f MATCHES "\\.dylib$" OR
                _f MATCHES "\\.pyc$" OR _f MATCHES "\\.class$")
            message(VERBOSE "get_source_files: filtered out (artifact ext): ${_f}")
            continue()
        endif()

        list(APPEND _filtered_sources "${_f}")
    endforeach()

    set(${out_sources} ${_filtered_sources} PARENT_SCOPE)
endfunction()


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
#            PRIVATE
            $<BUILD_INTERFACE:${dir}/src>
    )
endfunction()


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
            GTest::gtest
            GTest::gtest_main
            GTest::gmock
            GTest::gmock_main
    )

    boza_enable_warnings(${test_target})

    include(GoogleTest)
    gtest_discover_tests(${test_target} DISCOVERY_TIMEOUT 60)
endfunction()