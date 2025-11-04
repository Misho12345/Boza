enable_testing()

add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(ahi)
add_subdirectory(rhi)
add_subdirectory(engine)

add_custom_target(boza_all_tests
        COMMAND $<TARGET_FILE:boza_core_tests>
        COMMAND $<TARGET_FILE:boza_platform_tests>
        COMMAND $<TARGET_FILE:boza_ahi_tests>
        COMMAND $<TARGET_FILE:boza_rhi_tests>
        COMMAND $<TARGET_FILE:boza_engine_tests>
        USES_TERMINAL
        COMMENT "Build and run all test executables"
)

add_dependencies(boza_all_tests
        boza_core_tests
        boza_platform_tests
        boza_ahi_tests
        boza_rhi_tests
        boza_engine_tests
)

add_test(NAME AllTests COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target boza_all_tests)
