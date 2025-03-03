#include "Shader.hpp"

#include "Device.hpp"
#include "Logger.hpp"

namespace boza
{
    VkShaderModule Shader::create_shader_module(const std::string_view& filename)
    {
        const fs::path file_path = fs::path("shaders") / "spv" / std::format("{}.spv", filename);

        if (!exists(file_path))
        {
            Logger::critical("Shader file not found: {}", file_path.string());
            return nullptr;
        }

        std::vector<uint32_t> code = read_file(file_path.string());
        if (code.empty())
        {
            Logger::critical("Failed to read shader file: {}", file_path.string());
            return nullptr;
        }

        const VkShaderModuleCreateInfo create_info
        {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size() * sizeof(uint32_t),
            .pCode    = code.data()
        };

        VkShaderModule shader_module;
        VK_CHECK(vkCreateShaderModule(Device::get_device(), &create_info, nullptr, &shader_module),
        {
            LOG_VK_ERROR("Failed to create shader module");
            return nullptr;
        });

        return shader_module;
    }

    void Shader::destroy_shader_module(const VkShaderModule& shader_module)
    {
        vkDestroyShaderModule(Device::get_device(), shader_module, nullptr);
    }

    std::vector<uint32_t> Shader::read_file(const std::string_view& path)
    {
        std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};

        const size_t          size = file.tellg();
        std::vector<uint32_t> buffer((size + sizeof(uint32_t) - 1) / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        file.close();

        return buffer;
    }
}
