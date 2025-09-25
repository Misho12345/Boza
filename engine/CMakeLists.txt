add_compile_definitions(
        BOZAENGINE_EXPORTS=1
        BOZA_VERSION_MAJOR=1
        BOZA_VERSION_MINOR=0
        BOZA_VERSION_PATCH=0
)

add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(rhi)
add_subdirectory(ahi)
add_subdirectory(src)

if(BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
