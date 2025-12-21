boza_get_sources(sources ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(shader_processor ${sources})
add_executable(Tools::ShaderProcessor ALIAS shader_processor)

# === Find Dependencies ===
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(SPIRV-Tools-opt CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

# Find all spirv-cross components
foreach (component core glsl hlsl msl util cpp reflect)
    find_package(spirv_cross_${component} CONFIG REQUIRED)
endforeach ()

# === Configuration ===
target_include_directories(shader_processor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

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