add_executable(boza_game graphics_demo.cpp)
boza_enable_warnings(boza_game)

target_link_libraries(boza_game PRIVATE Boza::Engine)
add_dependencies(boza_game compile_shaders)
boza_link_assets(boza_game)
import_header_units(boza_game)

