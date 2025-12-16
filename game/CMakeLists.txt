add_executable(boza_game)

file(GLOB_RECURSE game_module_interfaces CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.ixx")
file(GLOB_RECURSE game_module_partitions CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")
file(GLOB_RECURSE game_module_implementations CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

target_include_directories(boza_game
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_sources(boza_game
        PRIVATE
        FILE_SET game_modules
        TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${game_module_interfaces} ${game_module_partitions}

        PRIVATE ${game_module_implementations}
)

boza_enable_warnings(boza_game)

target_link_libraries(boza_game PRIVATE Boza::Engine)
add_dependencies(boza_game compile_shaders)
boza_link_assets(boza_game)
import_header_units(boza_game)