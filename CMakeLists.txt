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

add_library(BozaEngine SHARED
        src/pch.hpp src/pch.cpp
        src/API.hpp
        src/Logger.hpp                      src/Logger.cpp
        src/Core/GameObject.hpp             src/Core/GameObject.cpp
        src/Core/Scene.hpp                  src/Core/Scene.cpp
        src/Core/Game.hpp                   src/Core/Game.cpp
        src/Core/Components/GameObjData.hpp
        src/Core/Components/Component.hpp
        src/Core/Components/Component.hpp
        src/Core/Components/Transform.hpp
)

target_precompile_headers(BozaEngine PRIVATE src/pch.hpp)
target_compile_definitions(BozaEngine PRIVATE BOZAENGINE_EXPORTS)
target_include_directories(BozaEngine PUBLIC src/)

find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
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

target_link_libraries(BozaEngine PUBLIC
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        glfw
        imgui::imgui
        glm::glm
        EnTT::EnTT
        box2d::box2d
        spdlog::spdlog
        OpenAL::OpenAL
        SndFile::sndfile
        ${LUA_LIBRARIES}
)

add_executable(BozaExample
        example/main.cpp
        example/pch.hpp example/pch.cpp
)

target_precompile_headers(BozaExample PRIVATE example/pch.hpp)
target_link_libraries(BozaExample PRIVATE BozaEngine)
