export module boza.rhi.vulkan:shader_module;

import boza.rhi.objects;
import <vk_all>;

export namespace boza::rhi::vk
{
    class ShaderModule final : public rhi::ShaderModule
    {
    public:
        ~ShaderModule() override { destroy(); }

        bool init() override;
        void destroy() override;

        [[nodiscard]] VkShaderModule vk_shader_module() const;

    private:
        explicit ShaderModule(const ShaderModuleDesc& desc) : rhi::ShaderModule(desc) {}
        VkShaderModule vk_shader_module_{ nullptr };

        friend GraphicsObject;
    };
}
