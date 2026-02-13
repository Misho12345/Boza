# === Target Declaration ===
add_executable(shader_processor)
add_executable(Tools::ShaderProcessor ALIAS shader_processor)
boza_enable_warnings(shader_processor)

# === Collect Module and Source Files ===
file(GLOB_RECURSE public_modules CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/public/*.ixx"
)

file(GLOB_RECURSE private_modules CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm"
)

file(GLOB_RECURSE implementations CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# === Module Configuration ===
boza_setup_module_target(shader_processor ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC_MODULES ${public_modules}
        PRIVATE_MODULES ${private_modules}
        SOURCES ${implementations}
)

target_sources(shader_processor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

# === Includes ===
target_include_directories(shader_processor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/public
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

# === Find Dependencies ===
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(SPIRV-Tools-opt CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

# Find all spirv-cross components
foreach (component core glsl hlsl msl util cpp reflect)
    find_package(spirv_cross_${component} CONFIG REQUIRED)
    target_link_libraries(shader_processor PRIVATE spirv-cross-${component})
endforeach ()

# === Link Libraries ===
target_link_libraries(shader_processor PRIVATE
        unofficial::shaderc::shaderc
        SPIRV-Tools-static
        SPIRV-Tools-opt
        nlohmann_json::nlohmann_json
)

# === Header Unit Library ===
set(include_dir ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

add_header_unit_library(shader_processor_header_units)

target_add_header_unit(shader_processor_header_units ${include_dir} nlohmann/json.hpp)
target_add_header_unit(shader_processor_header_units ${include_dir} shaderc/shaderc.hpp)
target_add_header_unit(shader_processor_header_units ${include_dir} spirv-tools/optimizer.hpp)
target_add_header_unit(shader_processor_header_units ${include_dir} spirv_cross/spirv_cross.hpp)

target_link_header_units(shader_processor shader_processor_header_units)
