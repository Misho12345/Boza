get_source_files(BOZA_GAME_SOURCES ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(boza_game ${BOZA_GAME_SOURCES})

target_link_libraries(boza_game PRIVATE Boza::Engine)
boza_enable_warnings(boza_game)

add_dependencies(boza_game compile_shaders)

boza_link_assets(boza_game)
boza_define_asset_paths(boza_game)
