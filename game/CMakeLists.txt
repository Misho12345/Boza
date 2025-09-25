add_executable(boza_game)

get_source_files(BOZA_GAME_SOURCES ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(boza_game PRIVATE ${BOZA_GAME_SOURCES})

target_link_libraries(boza_game PRIVATE Boza::Engine)
boza_enable_warnings(boza_game)

add_custom_target(copy_engine_dlls ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<TARGET_FILE_DIR:Boza::Engine>
        $<TARGET_FILE_DIR:boza_game>
        DEPENDS Boza::Engine
        COMMENT "Copying engine DLLs to game output folder"
)

add_dependencies(boza_game copy_engine_dlls)
