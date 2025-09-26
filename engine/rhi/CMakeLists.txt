add_library(boza_rhi STATIC)
add_library(Boza::Rhi ALIAS boza_rhi)

get_source_files(BOZA_RHI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(boza_rhi PRIVATE ${BOZA_RHI_SOURCES})

set_default_include_dirs(boza_rhi ${CMAKE_CURRENT_SOURCE_DIR})
boza_enable_warnings(boza_rhi)

find_package(Stb REQUIRED)
target_include_directories(boza_rhi PRIVATE ${Stb_INCLUDE_DIR})

target_link_libraries(boza_rhi
        PUBLIC
        Boza::Common
        Boza::Core
        Boza::Platform
)


if (BOZA_ENABLE_OPENGL)
    target_precompile_headers(boza_rhi PRIVATE src/backends/opengl/pch.hpp)
endif ()

if (BOZA_ENABLE_VULKAN)
    find_package(VulkanHeaders REQUIRED)
    find_package(VulkanMemoryAllocator CONFIG REQUIRED)
    find_package(volk CONFIG REQUIRED)

    target_link_libraries(boza_rhi PRIVATE
            Vulkan::Headers
            GPUOpen::VulkanMemoryAllocator
            volk::volk
            volk::volk_headers)

    if (APPLE)
        find_library(MOLTENVK_LIB MoltenVK PATHS
                "$ENV{HOMEBREW_PREFIX}/lib"
                "/opt/homebrew/lib"
                "/usr/local/lib"
                REQUIRED
        )

        target_link_libraries(boza_rhi PRIVATE
                "-framework Metal"
                "-framework QuartzCore"
                "-framework Foundation"
                "-framework IOSurface"
                "-framework CoreGraphics"
                "-framework AppKit"
                ${MOLTENVK_LIB}
        )

        get_filename_component(MVK_DIR "${MOLTENVK_LIB}" DIRECTORY)
        target_link_options(boza_rhi PRIVATE "-Wl,-rpath,${MVK_DIR}")
    endif ()

    target_precompile_headers(boza_rhi PRIVATE src/backends/vulkan/pch.hpp)
endif ()

if (BOZA_ENABLE_METAL)
    target_precompile_headers(boza_rhi PRIVATE src/backends/metal/pch.hpp)
endif ()

if (BOZA_ENABLE_DIRECTX11)
    target_precompile_headers(boza_rhi PRIVATE src/backends/dx11/pch.hpp)
endif ()

if (BOZA_ENABLE_DIRECTX12)
    target_precompile_headers(boza_rhi PRIVATE src/backends/dx12/pch.hpp)
endif ()


if (BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

