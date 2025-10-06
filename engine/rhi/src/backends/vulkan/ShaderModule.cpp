#include "ShaderModule.hpp"
#include "Device.hpp"
#include "boza/core/Logger.hpp"
#include "boza/AssetPaths.hpp"

namespace boza::rhi::vk
{
    bool ShaderModule::init()
    {
        Logger::trace("Creating vulkan shader module");

        const fs::path shader_dir = AssetPaths::get_shaders_dir();
        const fs::path path = shader_dir / desc.filename;
        const fs::path spv_path = path / (fs::path(desc.filename).stem().string() + ".spv");
        const std::vector<uint32_t> spv_data = read_file<uint32_t>(spv_path);

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
        VK_CHECK(vkCreateShaderModule(vk_device, &create_info, nullptr, &vk_shader_module_),
        {
            LOG_VK_ERROR("Failed to create shader module");
            return false;
        });

        if (!get_meta_data()) return false;

        return true;
    }

    void ShaderModule::destroy()
    {
        Logger::trace("Destroying vulkan shader module");

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (vk_shader_module_)
        {
            vkDestroyShaderModule(vk_device, vk_shader_module_, nullptr);
            vk_shader_module_ = nullptr;
        }
    }

    VkShaderModule ShaderModule::vk_shader_module() const { return vk_shader_module_; }
}
