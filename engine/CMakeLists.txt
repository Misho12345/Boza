add_compile_definitions(
        BOZA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        BOZA_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        BOZA_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

add_compile_definitions(BOZAENGINE_EXPORTS)


add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(rhi)
add_subdirectory(ahi)
add_subdirectory(src)

if (BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif ()