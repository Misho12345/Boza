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
        src/System.hpp

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

        src/Core/RenderingSystem/Vulkan/Instance.hpp
        src/Core/RenderingSystem/Vulkan/Instance.cpp
        src/Core/RenderingSystem/Vulkan/Device.hpp
        src/Core/RenderingSystem/Vulkan/Device.cpp
        src/Core/RenderingSystem/Vulkan/Swapchain.hpp
        src/Core/RenderingSystem/Vulkan/Swapchain.cpp
        src/Core/RenderingSystem/Vulkan/Shader.hpp
        src/Core/RenderingSystem/Vulkan/Shader.cpp
        src/Core/RenderingSystem/Vulkan/Pipeline.hpp
        src/Core/RenderingSystem/Vulkan/Pipeline.cpp
        src/Core/RenderingSystem/Vulkan/Memory/Buffer.hpp
        src/Core/RenderingSystem/Vulkan/Memory/Buffer.cpp
        src/Core/RenderingSystem/Vulkan/Memory/Allocator.hpp
        src/Core/RenderingSystem/Vulkan/Memory/Allocator.cpp
        src/Core/RenderingSystem/Vulkan/CommandPool.hpp
        src/Core/RenderingSystem/Vulkan/CommandPool.cpp

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
        src/Core/RenderingSystem/Vulkan/Memory/DescriptorPool.cpp
        src/Core/RenderingSystem/Vulkan/Memory/DescriptorPool.hpp
        src/Core/RenderingSystem/Vulkan/Memory/DescriptorSetLayout.cpp
        src/Core/RenderingSystem/Vulkan/Memory/DescriptorSetLayout.hpp
)

target_precompile_headers(BozaEngine PRIVATE src/boza_pch.hpp)
target_compile_definitions(BozaEngine PRIVATE BOZAENGINE_EXPORTS)
target_include_directories(BozaEngine PUBLIC src/)

find_package(VulkanHeaders REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(glfw3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(box2d CONFIG REQUIRED)
find_package(OpenAL REQUIRED)
find_package(SndFile REQUIRED)
find_package(Stb REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(Taskflow CONFIG REQUIRED)

target_link_libraries(BozaEngine PUBLIC
        Vulkan::Headers
        GPUOpen::VulkanMemoryAllocator
        volk::volk volk::volk_headers
        glfw
        imgui::imgui
        glm::glm-header-only
        EnTT::EnTT
        box2d::box2d
        fmt::fmt-header-only
        spdlog::spdlog_header_only
        OpenAL::OpenAL
        SndFile::sndfile
        ${LUA_LIBRARIES}
        Taskflow::Taskflow
)

add_executable(BozaExample
        example/main.cpp
        example/example_pch.hpp example/example_pch.cpp
)

target_precompile_headers(BozaExample PRIVATE example/example_pch.hpp)
target_link_libraries(BozaExample PRIVATE BozaEngine)
