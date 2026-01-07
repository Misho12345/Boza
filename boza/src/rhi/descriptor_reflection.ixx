export module boza.rhi:descriptor_reflection;

import std;
import boza.common;
import boza.gfx;
import boza.rhi.objects;

export namespace boza::rhi
{
    struct BindingInfo
    {
        std::uint32_t set{ 0 };
        std::uint32_t binding{ 0 };
        std::uint32_t offset{ 0 };
        std::uint32_t size{ 0 };
        DescriptorType descriptor_type{ DescriptorType::UniformBuffer };
        ShaderDataType data_type{ ShaderDataType::Unknown };
        bool is_push_constant{ false };
    };

    class DescriptorReflection final
    {
    public:
        DescriptorReflection() = default;

        void build_from_shaders(std::span<ShaderModule*> shaders);

        [[nodiscard]]
        std::optional<BindingInfo> lookup(std::string_view name) const;

        [[nodiscard]]
        const flat_map<std::string, BindingInfo>& bindings() const { return bindings_; }

    private:
        flat_map<std::string, BindingInfo> bindings_;

        void add_uniform_buffer_members(
            const std::string& buffer_name,
            const ShaderModule::ShaderResource& resource);

        void add_push_constant_members(
            const std::string& pc_name,
            const ShaderModule::PushConstant& pc);
    };
}

