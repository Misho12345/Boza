#pragma once
#include "pch.hpp"
#include "boza/rhi/ShaderModule.hpp"

namespace boza::rhi::vk
{
    class ShaderModule final : public rhi::ShaderModule
    {
    public:
        bool init() override;
        void destroy() override;

        [[nodiscard]] VkShaderModule vk_shader_module() const;

    private:
        explicit ShaderModule(const ShaderModuleDesc& desc) : rhi::ShaderModule(desc) {}
        VkShaderModule vk_shader_module_{ nullptr };

        friend GraphicsObject;
    };
}

