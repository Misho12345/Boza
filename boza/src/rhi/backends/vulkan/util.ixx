module;

#include <vulkan/vulkan.h>

export module boza.rhi.vulkan:util;

import std;
import boza.core;

export namespace boza::rhi::vk
{
    constexpr inline auto vk_api_version_1_3                            = VK_API_VERSION_1_3;
    constexpr inline auto vk_ext_debug_utils_extension_name             = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    constexpr inline auto vk_khr_portability_enumeration_extension_name = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
    constexpr inline auto vk_khr_validation_layer_name                  = "VK_LAYER_KHRONOS_validation";
    constexpr inline auto vk_queue_family_ignored                       = VK_QUEUE_FAMILY_IGNORED;
    constexpr inline auto vk_log_clamp_none                             = VK_LOD_CLAMP_NONE;

    constexpr inline auto vk_pipeline_stage_2_top_of_pipe_bit = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    constexpr inline auto vk_pipeline_stage_2_bottom_of_pipe_bit = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    constexpr inline auto vk_pipeline_stage_2_color_attachment_output_bit = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    constexpr inline auto vk_access_2_color_attachment_write_bit = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;


    constexpr auto vk_make_version(const auto& major, const auto& minor, const auto& patch)
    {
        return VK_MAKE_VERSION(major, minor, patch);
    }

    constexpr auto vk_api_version_major(const auto& version) { return VK_VERSION_MAJOR(version); }
    constexpr auto vk_api_version_minor(const auto& version) { return VK_VERSION_MINOR(version); }
    constexpr auto vk_api_version_patch(const auto& version) { return VK_VERSION_PATCH(version); }

    template<typename... Args>
    [[nodiscard]] bool vk_check(const VkResult result, const std::format_string<Args...> fmt, Args&&... args)
    {
        if (result == VK_SUCCESS) return true;
        Log::error(fmt, std::forward<Args>(args)...);
        return false;
    }


    [[nodiscard]] bool vk_check(const VkResult result, const auto& msg) { return vk_check(result, "{}", msg); }
}
