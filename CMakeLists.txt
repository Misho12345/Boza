cmake_minimum_required(VERSION 3.30)
project(Boza)

set(CMAKE_CXX_STANDARD 26)

if (DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")
endif ()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(NDEBUG)
endif()

if(MSVC)
    add_compile_options(/Zc:preprocessor)
endif ()

#if(MSVC)
#    add_compile_options(/W4 /WX /Ox)
#elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#    add_compile_options(-Wall -Wextra -Werror -O3)
#endif()

add_library(BozaEngine SHARED
        src/boza_pch.hpp
        src/boza_pch.cpp
        src/API.hpp

        src/Logger.hpp
        src/Logger.inl
        src/Logger.cpp

        src/Singleton.hpp

        src/SystemBase.hpp
        src/FixedSystem.hpp
        src/VariableSystem.hpp

        src/Core/App.hpp
        src/Core/App.cpp

        src/Core/JobSystem/JobSystem.cpp
        src/Core/JobSystem/JobSystem.hpp

        src/Core/EventSystem/EventSystem.hpp
        src/Core/EventSystem/EventSystem.inl

        src/Core/InputSystem/InputSystem.hpp
        src/Core/InputSystem/InputSystem.cpp

        src/Core/PhysicsSystem/PhysicsSystem.hpp
        src/Core/PhysicsSystem/PhysicsSystem.cpp

        src/Core/RenderingSystem/RenderingSystem.hpp
        src/Core/RenderingSystem/RenderingSystem.cpp

        src/Render/Renderer.cpp
        src/Render/Renderer.hpp
        src/GPU/Vulkan/Core/Instance.hpp
        src/GPU/Vulkan/Core/Instance.cpp
        src/GPU/Vulkan/Core/Device.hpp
        src/GPU/Vulkan/Core/Device.cpp
        src/GPU/Vulkan/Core/Swapchain.hpp
        src/GPU/Vulkan/Core/Swapchain.cpp
        src/GPU/Vulkan/Pipeline/ShaderLoader.hpp
        src/GPU/Vulkan/Pipeline/ShaderLoader.cpp
        src/GPU/Vulkan/Pipeline/Pipeline.hpp
        src/GPU/Vulkan/Pipeline/Pipeline.cpp
        src/GPU/Vulkan/Pipeline/PipelineManager.cpp
        src/GPU/Vulkan/Pipeline/PipelineManager.hpp
        src/GPU/Vulkan/Core/CommandPool.hpp
        src/GPU/Vulkan/Core/CommandPool.cpp
        src/Render/Mesh.cpp
        src/Render/Mesh.hpp
        src/Render/Mesh.inl
        src/GPU/Vulkan/Memory/Buffer.hpp
        src/GPU/Vulkan/Memory/Buffer.cpp
        src/GPU/Vulkan/Memory/Allocator.hpp
        src/GPU/Vulkan/Memory/Allocator.cpp
        src/GPU/Vulkan/Descriptor/DescriptorPool.cpp
        src/GPU/Vulkan/Descriptor/DescriptorPool.hpp
        src/Render/MeshManager.cpp
        src/Render/MeshManager.hpp
        src/Render/MeshManager.inl
        src/GPU/Vulkan/Descriptor/DescriptorSet.cpp
        src/GPU/Vulkan/Descriptor/DescriptorSet.hpp
        src/GPU/Vulkan/Descriptor/DescriptorSet.inl
        src/GPU/Vulkan/Memory/Texture.cpp
        src/GPU/Vulkan/Memory/Texture.hpp

        src/Core/GameObject.hpp
        src/Core/GameObject.inl
        src/Core/GameObject.cpp

        src/Core/Scene.hpp
        src/Core/Scene.cpp

        src/Core/Window.hpp
        src/Core/Window.cpp

        src/Core/Components/Component.hpp
        src/Core/Components/GameObjData.hpp
        src/Core/Components/Transform.hpp
        src/Core/Components/Behaviour.hpp
        src/GPU/Vulkan/Vertex/VertexLayout.hpp
        src/Serialize.hpp
)

target_precompile_headers(BozaEngine PRIVATE src/boza_pch.hpp)
target_compile_definitions(BozaEngine PRIVATE BOZAENGINE_EXPORTS)
target_include_directories(BozaEngine PUBLIC src/)

find_package(VulkanHeaders REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(SPIRV-Tools-opt CONFIG REQUIRED)
find_package(glfw3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(box2d CONFIG REQUIRED)
find_package(OpenAL REQUIRED)
find_package(SndFile REQUIRED)
find_package(Stb REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS pfr preprocessor)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(BozaEngine PUBLIC
        Vulkan::Headers
        GPUOpen::VulkanMemoryAllocator
        volk::volk volk::volk_headers
        unofficial::spirv-reflect
        unofficial::shaderc::shaderc
        SPIRV-Tools-static
        SPIRV-Tools-opt
        glfw
        imgui::imgui
        glm::glm-header-only
        EnTT::EnTT
        box2d::box2d
        fmt::fmt-header-only
        spdlog::spdlog_header_only
        OpenAL::OpenAL
        SndFile::sndfile
        Taskflow::Taskflow
        magic_enum::magic_enum
        Boost::pfr
        Boost::preprocessor
        nlohmann_json::nlohmann_json
)

add_executable(BozaExample
        example/main.cpp
        example/example_pch.hpp example/example_pch.cpp
)

target_precompile_headers(BozaExample PRIVATE example/example_pch.hpp)
target_link_libraries(BozaExample PRIVATE BozaEngine)
