add_library(boza_engine SHARED)
add_library(Boza::Engine ALIAS boza_engine)

get_source_files(BOZA_ENGINE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(boza_engine PRIVATE ${BOZA_ENGINE_SOURCES})

target_include_directories(boza_engine
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

boza_enable_warnings(boza_engine)

target_link_libraries(boza_engine
        PUBLIC
        Boza::Common
        $<TARGET_OBJECTS:Boza::Core>
        PRIVATE
        Boza::Platform
        Boza::Rhi
        Boza::Ahi
)

add_custom_command(TARGET boza_engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        $<TARGET_FILE_DIR:boza_engine>
        $<TARGET_FILE_DIR:${BOZA_GAME_TARGET}>
        COMMENT "Copying engine dll to game output folder"
        VERBATIM
)
 