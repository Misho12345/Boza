include_guard(GLOBAL)

function(boza_enable_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE
                /W4
                /WX
                /permissive-
                /wd4702 # unreachable code
                /wd4065 # switch with 'default' but no 'case'
                /wd4251 # DLL-interface warning
                /wd5050 # modules compatibility
                /wd4127 # conditional expression is constant
                /wd4324 # structure was padded due to alignment specifier
                /wd4244 # implicit conversion with possible loss of data
        )

        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.30")
            target_compile_options(${target} PRIVATE /wd5105)
        endif ()
    else ()
        # Common GCC/Clang warnings
        target_compile_options(${target} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Werror
                -Wconversion
                -Wsign-conversion
                -Wno-missing-field-initializers
                -Wcast-align
                -Wunused
                -Wold-style-cast
        )

        # GCC-specific warnings
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE
                    -Wlogical-op
                    -Wduplicated-cond
                    -Wduplicated-branches
                    -Wnull-dereference
                    -Wdouble-promotion
            )
        endif ()

        # Clang-specific warnings
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${target} PRIVATE
                    -Wmost
                    -Wextra-semi
                    -Wcomma
                    -Wnon-virtual-dtor
            )
        endif ()
    endif ()
endfunction()