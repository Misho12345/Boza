function(boza_enable_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE
                /W4
                /permissive-
                /wd4702
                /wd4065
                /wd4251
        )

        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.30")
            target_compile_options(${target} PRIVATE /wd5105)
        endif ()

        target_compile_options(${target} PRIVATE /WX)
    else ()
        target_compile_options(${target} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wno-missing-field-initializers
                -Wcast-align
                -Wunused
                -Wold-style-cast
        )

        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE
                    -Wlogical-op
                    -Wduplicated-cond
                    -Wduplicated-branches
                    -Wnull-dereference
                    -Wdouble-promotion
            )
        endif ()

        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_compile_options(${target} PRIVATE
                    -Wmost
                    -Wextra-semi
                    -Wcomma
                    -Wnon-virtual-dtor
            )
        endif ()

        target_compile_options(${target} PRIVATE -Werror)
    endif ()
endfunction()