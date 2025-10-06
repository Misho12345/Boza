add_library(boza_platform_interface INTERFACE)
add_library(Boza::PlatformInterface ALIAS boza_platform_interface)

get_source_files(BOZA_PLATFORM_INTERFACE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_sources(boza_platform_interface INTERFACE ${BOZA_PLATFORM_INTERFACE_SOURCES})

target_include_directories(boza_platform_interface
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(boza_platform_interface INTERFACE Boza::Common)



add_library(boza_platform STATIC)
add_library(Boza::Platform ALIAS boza_platform)

get_source_files(BOZA_PLATFORM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(boza_platform PRIVATE ${BOZA_PLATFORM_SOURCES})

set_default_include_dirs(boza_platform ${CMAKE_CURRENT_SOURCE_DIR})
boza_enable_warnings(boza_platform)

find_package(glfw3 REQUIRED)

if (BOZA_ENABLE_VULKAN)
    find_package(VulkanHeaders REQUIRED)
    target_link_libraries(boza_platform PRIVATE Vulkan::Headers)
endif()

target_link_libraries(boza_platform
        PUBLIC
        Boza::Common
        Boza::Core
        Boza::PlatformInterface
        PRIVATE
        glfw
)

if(BOZA_BUILD_TESTS)
    add_subdirectory(tests)
endif()
