#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class ShaderLoader final
    {
    public:
        [[nodiscard]]
        static VkShaderModule create_shader_module(const std::string_view& filename);
        static void destroy_shader_module(const VkShaderModule& shader_module);

    private:
        [[nodiscard]]
        static std::vector<uint32_t> read_file(const std::string_view& path);
    };
}
