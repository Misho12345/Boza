# Changed from SHARED to STATIC to workaround some EnTT type erasure issues
# TODO: Change to SHARED or when I figure out how to fix the issues
add_library(boza_engine STATIC)
add_library(Boza::Engine ALIAS boza_engine)
boza_enable_warnings(boza_engine)

file(GLOB_RECURSE public_module_interfaces CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.ixx")
file(GLOB_RECURSE private_module_interfaces CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx")
file(GLOB_RECURSE private_module_partitions CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm")
file(GLOB_RECURSE module_implementations CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

target_compile_definitions(boza_engine
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

target_include_directories(boza_engine
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_sources(boza_engine
        PUBLIC
        FILE_SET boza_modules_public
        TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${public_module_interfaces}

        PRIVATE
        FILE_SET boza_modules_private
        TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
        FILES ${private_module_interfaces} ${private_module_partitions}

        PRIVATE ${module_implementations}
)

find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

target_sources(boza_engine
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${glm_DIR}../../../include/glm
        FILES ${glm_DIR}/../../include/glm/glm.cppm
)

target_link_libraries(boza_engine
        PUBLIC
        EnTT::EnTT
        spdlog::spdlog
        glfw
)

set(include_dir ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

set(ENTT_HEADER_UNIT_DEFS ENTT_STANDARD_CPP)

create_header_unit(${include_dir} entt/entt.hpp COMPILE_DEFINITIONS ${ENTT_HEADER_UNIT_DEFS})
create_header_unit(${include_dir} nlohmann/json.hpp)
create_header_unit(${include_dir} stb_image.h)
create_header_unit(${include_dir} stb_image_write.h)

if (BOZA_ENABLE_OPENGL)
    # Do OpenGL stuff
endif ()

if (BOZA_ENABLE_VULKAN)
    set(VK_ALL_HEADER_UNIT_DEFS
            VK_NO_PROTOTYPES
            VMA_STATIC_VULKAN_FUNCTIONS=0
            VMA_DYNAMIC_VULKAN_FUNCTIONS=1
    )

    find_package(VulkanHeaders REQUIRED)
    find_package(VulkanMemoryAllocator CONFIG REQUIRED)
    find_package(volk CONFIG REQUIRED)

    set(VMA_IMPL "${CMAKE_CURRENT_BINARY_DIR}/vma_impl.cpp")
    file(WRITE ${VMA_IMPL}
            "
            #define VMA_IMPLEMENTATION
            #if __has_include(<vma/vk_mem_alloc.h>)
            #include <vma/vk_mem_alloc.h>
            #elif __has_include(<vk_mem_alloc.h>)
            #include <vk_mem_alloc.h>
            #else
            #error \"Cannot find vk_mem_alloc.h\"
            #endif
            ")
    target_sources(boza_engine PRIVATE ${VMA_IMPL})

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include/vk_all.h
            "
            #if __has_include(<vma/vk_mem_alloc.h>)
            #include <vma/vk_mem_alloc.h>
            #elif __has_include(<vk_mem_alloc.h>)
            #include <vk_mem_alloc.h>
            #else
            #error \"Cannot find vk_mem_alloc.h\"
            #endif

            #include <volk.h>
            ")
    target_include_directories(boza_engine PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)

    target_link_libraries(boza_engine PUBLIC
            Vulkan::Headers
            GPUOpen::VulkanMemoryAllocator
            volk::volk
            volk::volk_headers
    )

    create_header_unit(${CMAKE_CURRENT_BINARY_DIR}/include vk_all.h COMPILE_DEFINITIONS ${VK_ALL_HEADER_UNIT_DEFS} INCLUDE_DIRECTORIES ${include_dir})
    create_header_unit(${include_dir} vulkan/vulkan.h COMPILE_DEFINITIONS ${VK_ALL_HEADER_UNIT_DEFS})
endif ()

if (BOZA_ENABLE_METAL)
    # Do Metal stuff
endif ()

if (BOZA_ENABLE_DIRECTX11)
    # Do DirectX11 stuff
endif ()

if (BOZA_ENABLE_DIRECTX12)
    # Do DirectX12 stuff
endif ()

set(STB_IMPL "${CMAKE_CURRENT_BINARY_DIR}/stb_impl.cpp")
file(WRITE ${STB_IMPL}
        "
        #define STB_IMAGE_IMPLEMENTATION
        #include <stb_image.h>

        #define STB_IMAGE_WRITE_IMPLEMENTATION
        #include <stb_image_write.h>
        ")
target_sources(boza_engine PRIVATE ${STB_IMPL})

import_header_units(boza_engine APPLY_DEFINITIONS)
