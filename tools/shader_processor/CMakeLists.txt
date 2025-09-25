get_source_files(SHADER_COMPILER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(shader_processor ${SHADER_COMPILER_SOURCES})

find_package(unofficial-shaderc CONFIG REQUIRED)

find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(SPIRV-Tools-opt CONFIG REQUIRED)

find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_hlsl CONFIG REQUIRED)
find_package(spirv_cross_msl CONFIG REQUIRED)
find_package(spirv_cross_util CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)

find_package(nlohmann_json REQUIRED)


target_include_directories(shader_processor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(shader_processor PRIVATE
        unofficial::shaderc::shaderc

        SPIRV-Tools-static
        SPIRV-Tools-opt

        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-util
        spirv-cross-cpp
        spirv-cross-reflect

        nlohmann_json::nlohmann_json
)
