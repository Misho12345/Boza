export module boza.rhi:descriptor_reflection;

import std;
import boza.common;
import boza.gfx.common;
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

    struct PushConstantRangeInfo
    {
        std::uint32_t offset{ 0 };
        std::uint32_t size{ 0 };
        Flags<ShaderStage> stages{};
        std::vector<ShaderModule::PushConstantMember> members;
    };

    struct ResourceInfo
    {
        std::uint32_t set{ 0 };
        std::uint32_t binding{ 0 };
        std::uint32_t size{ 0 };
        DescriptorType descriptor_type{ DescriptorType::UniformBuffer };
        ShaderDataType data_type{ ShaderDataType::Unknown };
        Flags<ShaderStage> stages{};
        std::vector<ShaderModule::PushConstantMember> members;
    };

    struct StructTypeInfo
    {
        std::uint32_t size{ 0 };
        std::vector<ShaderModule::PushConstantMember> members;
    };

    class DescriptorReflection final
    {
    public:
        DescriptorReflection() = default;

        void build_from_shaders(std::span<ShaderModule*> shaders);

        [[nodiscard]]
        std::optional<BindingInfo> lookup(std::string_view name) const;

        [[nodiscard]] const flat_map<std::string, BindingInfo>& bindings() const { return bindings_; }
        [[nodiscard]] const flat_map<std::string, PushConstantRangeInfo>& push_constant_ranges() const { return push_constant_ranges_; }
        [[nodiscard]] const flat_map<std::string, ResourceInfo>& storage_buffers() const { return storage_buffers_; }
        [[nodiscard]] const flat_map<std::string, StructTypeInfo>& struct_types() const { return struct_types_; }

    private:
        flat_map<std::string, BindingInfo> bindings_;
        flat_map<std::string, PushConstantRangeInfo> push_constant_ranges_;
        flat_map<std::string, ResourceInfo> storage_buffers_;
        flat_map<std::string, StructTypeInfo> struct_types_;

        void add_uniform_buffer_members(
            const std::string& buffer_name,
            const ShaderModule::ShaderResource& resource);

        void add_storage_buffer(
            const std::string& buffer_name,
            const ShaderModule::ShaderResource& resource,
            Flags<ShaderStage> stages);

        void add_push_constant_range(
            const std::string& range_name,
            const ShaderModule::PushConstant& pc,
            Flags<ShaderStage> stages);

        void add_push_constant_members(
            const std::string& range_name,
            const PushConstantRangeInfo& range_info);

        void merge_struct_types(const flat_map<std::string, ShaderModule::StructType>& struct_types);
    };
}
