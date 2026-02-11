add_library(boza_engine STATIC)
add_library(Boza::Engine ALIAS boza_engine)
boza_enable_warnings(boza_engine)

# === Collect Module Files ===
file(GLOB_RECURSE public_modules CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/public/*.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/public/*.cppm"
)
file(GLOB_RECURSE private_modules CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm"
)
file(GLOB_RECURSE implementations CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# === Module Configuration ===
boza_setup_module_target(boza_engine ${CMAKE_CURRENT_SOURCE_DIR}
        PUBLIC_MODULES ${public_modules}
        PRIVATE_MODULES ${private_modules}
        SOURCES ${implementations}
)

target_include_directories(boza_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/public
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

# === Engine Definitions ===
target_compile_definitions(boza_engine
        PUBLIC
        BOZA_PROJECT_ROOT=\"${CMAKE_SOURCE_DIR}\"

        PRIVATE
        BOZAENGINE_EXPORTS
        BOZA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        BOZA_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        BOZA_VERSION_PATCH=${PROJECT_VERSION_PATCH}

        GLM_FORCE_RADIANS
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_INLINE
        GLM_FORCE_CTOR_INIT
        GLM_FORCE_LEFT_HANDED
)

# === Core Dependencies ===
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(flecs CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

target_sources(boza_engine
        PUBLIC FILE_SET CXX_MODULES
        BASE_DIRS ${glm_DIR}../../../include/glm
        FILES ${glm_DIR}/../../include/glm/glm.cppm
)

target_link_libraries(boza_engine PUBLIC
        flecs::flecs
        spdlog::spdlog
        glfw
)

# === Header Units Setup ===
set(include_dir ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
add_header_unit_library(boza_engine_header_units)

target_add_header_unit(boza_engine_header_units ${include_dir} flecs.h)
target_add_header_unit(boza_engine_header_units ${include_dir} gtl/phmap.hpp)
target_add_header_unit(boza_engine_header_units ${include_dir} nlohmann/json.hpp)
target_add_header_unit(boza_engine_header_units ${include_dir} stb_image.h)
target_add_header_unit(boza_engine_header_units ${include_dir} stb_image_write.h)

# === STB Implementation ===
boza_generate_impl(boza_engine
        "#define STB_IMAGE_IMPLEMENTATION
        #include <stb_image.h>
        #define STB_IMAGE_WRITE_IMPLEMENTATION
        #include <stb_image_write.h>"

        "stb_impl.cpp")

# === Graphics Backends ===
if (BOZA_ENABLE_VULKAN)
    find_package(VulkanHeaders REQUIRED)
    find_package(VulkanMemoryAllocator CONFIG REQUIRED)
    find_package(volk CONFIG REQUIRED)

    # VMA implementation
    boza_generate_impl(boza_engine
            "#define VMA_IMPLEMENTATION
            #if __has_include(<vma/vk_mem_alloc.h>)
            #include <vma/vk_mem_alloc.h>
            #elif __has_include(<vk_mem_alloc.h>)
            #include <vk_mem_alloc.h>
            #else
            #error \"Cannot find vk_mem_alloc.h\"
            #endif"

            "vma_impl.cpp")

    # Unified Vulkan header
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include/vk_all
            "#if __has_include(<vma/vk_mem_alloc.h>)
            #include <vma/vk_mem_alloc.h>
            #elif __has_include(<vk_mem_alloc.h>)
            #include <vk_mem_alloc.h>
            #else
            #error \"Cannot find vk_mem_alloc.h\"
            #endif
            #include <volk.h>")

    target_link_libraries(boza_engine PUBLIC
            Vulkan::Headers
            GPUOpen::VulkanMemoryAllocator
            volk::volk
            volk::volk_headers
    )

    # Vulkan header units
    set(VK_DEFS
            VK_NO_PROTOTYPES
            VMA_STATIC_VULKAN_FUNCTIONS=0
            VMA_DYNAMIC_VULKAN_FUNCTIONS=1
    )

    target_add_header_unit(boza_engine_header_units
            ${CMAKE_CURRENT_BINARY_DIR}/include vk_all
            COMPILE_DEFINITIONS ${VK_DEFS}
            INCLUDE_DIRECTORIES ${include_dir}
    )
endif ()

# === Link Header Units ===
target_link_header_units(boza_engine boza_engine_header_units)