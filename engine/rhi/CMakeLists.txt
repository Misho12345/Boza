add_library(boza_rhi_interface INTERFACE)
get_source_files(BOZA_RHI_INTERFACE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(boza_rhi_interface INTERFACE ${BOZA_RHI_INTERFACE_SOURCES})

target_include_directories(boza_rhi_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Stb REQUIRED)

target_include_directories(boza_rhi_interface INTERFACE ${Stb_INCLUDE_DIR})

target_link_libraries(boza_rhi_interface INTERFACE
        Boza::Common
        Boza::CoreInterface
        Boza::PlatformInterface
)



add_library(boza_rhi STATIC)
add_library(Boza::Rhi ALIAS boza_rhi)

get_source_files(BOZA_RHI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/rhi)
target_sources(boza_rhi PRIVATE ${BOZA_RHI_SOURCES})

set_default_include_dirs(boza_rhi ${CMAKE_CURRENT_SOURCE_DIR})
boza_enable_warnings(boza_rhi)

target_link_libraries(boza_rhi PUBLIC boza_rhi_interface)


if (BOZA_ENABLE_OPENGL)
    setup_backend(boza_rhi_opengl boza_rhi_interface boza_rhi ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/opengl)
endif ()

if (BOZA_ENABLE_VULKAN)
    setup_backend(boza_rhi_vulkan boza_rhi_interface boza_rhi ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/vulkan)

    find_package(VulkanHeaders REQUIRED)
    find_package(VulkanMemoryAllocator CONFIG REQUIRED)
    find_package(volk CONFIG REQUIRED)

    # Create a separate static library for VMA implementation
    # This is done to avoid issues with the precompiled header in boza_rhi_vulkan that has vma included

    set(VMA_IMPL_FILE "${CMAKE_CURRENT_BINARY_DIR}/vma_implementation.cpp")
    file(WRITE ${VMA_IMPL_FILE}
            "
            #define VMA_STATIC_VULKAN_FUNCTIONS 0
            #define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
            #define VMA_IMPLEMENTATION
            #if __has_include(<vk_mem_alloc.h>)
            #include <vk_mem_alloc.h>
            #elif __has_include(<vma/vk_mem_alloc.h>)
            #include <vma/vk_mem_alloc.h>
            #else
            #error \"Cannot find vk_mem_alloc.h\"
            #endif
            ")

    add_library(boza_vma_impl STATIC ${VMA_IMPL_FILE})
    target_link_libraries(boza_vma_impl PUBLIC
            Vulkan::Headers
            GPUOpen::VulkanMemoryAllocator
            volk::volk
            volk::volk_headers
    )


    target_link_libraries(boza_rhi_vulkan PRIVATE boza_vma_impl)


    if (APPLE)
        find_library(MOLTENVK_LIB MoltenVK PATHS
                "$ENV{HOMEBREW_PREFIX}/lib"
                "/opt/homebrew/lib"
                "/usr/local/lib"
                REQUIRED
        )

        target_link_libraries(boza_rhi_vulkan PRIVATE
                "-framework Metal"
                "-framework QuartzCore"
                "-framework Foundation"
                "-framework IOSurface"
                "-framework CoreGraphics"
                "-framework AppKit"
                ${MOLTENVK_LIB}
        )

        get_filename_component(MVK_DIR "${MOLTENVK_LIB}" DIRECTORY)
        target_link_options(boza_rhi_vulkan PRIVATE "-Wl,-rpath,${MVK_DIR}")
    endif ()
endif ()

if (BOZA_ENABLE_METAL)
    setup_backend(boza_rhi_metal boza_rhi_interface boza_rhi ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/metal)
endif ()

if (BOZA_ENABLE_DIRECTX11)
    setup_backend(boza_rhi_dx11 boza_rhi_interface boza_rhi ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/dx11)
endif ()

if (BOZA_ENABLE_DIRECTX12)
    setup_backend(boza_rhi_dx12 boza_rhi_interface boza_rhi ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/dx12)
endif ()





if (BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
