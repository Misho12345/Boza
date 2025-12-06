module boza.rhi.vulkan;

import :util;
import boza.detail;

namespace boza::rhi::vk
{
    using detail::AssetPaths;

    bool ShaderModule::init()
    {
        const fs::path shader_dir = AssetPaths::shaders_dir();
        const fs::path shader_subdir = shader_dir / desc.filename;

        const fs::path shader_name = fs::path(desc.filename).stem();
        const fs::path spv_path = shader_subdir / (shader_name.string() + ".spv");

        const std::vector<std::uint32_t> spv_data = read_file<std::uint32_t>(spv_path);

        if (spv_data.empty()) return false;

        const VkShaderModuleCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .codeSize = static_cast<uint32_t>(spv_data.size()) * sizeof(uint32_t),
            .pCode = spv_data.data()
        };

        const auto vk_device = static_cast<Device*>(desc.device)->logical_device();
        if (!vk_check(
            vkCreateShaderModule(vk_device, &create_info, nullptr, &vk_shader_module_),
            "Failed to create shader module"))
            return false;

        if (!get_meta_data()) return false;

        return true;
    }

    void ShaderModule::destroy()
    {
        // Log::trace("Destroying vulkan shader module");

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (vk_shader_module_)
        {
            vkDestroyShaderModule(vk_device, vk_shader_module_, nullptr);
            vk_shader_module_ = nullptr;
        }
    }

    VkShaderModule ShaderModule::vk_shader_module() const { return vk_shader_module_; }
}