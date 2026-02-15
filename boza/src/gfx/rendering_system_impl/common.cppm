module boza.gfx:rendering_system_common;

import std;
import boza.common;
import boza.core;
import boza.rhi;

import :rendering_system;
import :buffer;

namespace boza
{
    struct PushConstantRangeRuntime
    {
        std::string range_name{};
        std::uint32_t offset{ 0 };
        std::uint32_t size{ 0 };
        Flags<rhi::ShaderStage> stages{};

        bool has_model_field{ false };
        std::uint32_t model_offset_in_range{ 0 };

        bool has_use_instancing_field{ false };
        std::uint32_t use_instancing_offset_in_range{ 0 };

        bool has_data_member{ false };
        std::string data_member_name{};
        std::string data_type_name{};
        std::uint32_t data_offset_in_range{ 0 };
        std::uint32_t data_size{ 0 };
        std::uint32_t data_struct_size{ 0 };

        bool instancing_supported{ false };
        std::string instancing_ssbo_name{};
        std::uint32_t instancing_stride{ 0 };
    };

    struct MaterialRenderInfo
    {
        std::vector<PushConstantRangeRuntime> ranges{};
        std::optional<std::size_t> instancing_range_index{};
    };

    struct InstanceBufferState
    {
        std::unique_ptr<Buffer> buffer{};
        std::size_t capacity_bytes{ 0 };
        void* bound_handle{ nullptr };
    };

    flat_map<Material*, MaterialRenderInfo> material_render_infos_{};
    flat_map<Material*, flat_map<std::string, InstanceBufferState>> instance_buffers_{};
    std::vector<std::uint8_t> push_constant_scratch_{};
    std::vector<std::uint8_t> instance_payload_scratch_{};

    constexpr std::size_t initial_instance_buffer_bytes_ = 65'536 * sizeof(glm::mat4);
    constexpr std::string_view model_field_name_ = "model";
    constexpr std::string_view instancing_field_name_ = "use_instancing";

    void reset_render_caches();

    MaterialRenderInfo build_material_render_info(Material* material);
    MaterialRenderInfo* get_or_build_render_info(Material* material);

    void push_ranges(
        rhi::CommandBuffer* cmd,
        Material& material,
        const MaterialRenderInfo* info,
        const glm::mat4* model,
        bool use_instancing);

    bool upload_instance_payload(
        Material* material,
        const MeshBucket& bucket,
        const PushConstantRangeRuntime& range);

    void ensure_fallback_ssbo_bound(Material& material, const MaterialRenderInfo* info);

    bool validate_instancing(
        Material* material,
        const std::string& range_name,
        const rhi::PushConstantRangeInfo& range_info,
        PushConstantRangeRuntime& runtime_range,
        const std::vector<const rhi::ShaderModule::PushConstantMember*>& data_members,
        const rhi::ShaderModule::PushConstantMember* use_instancing_member,
        const flat_map<std::string, rhi::ResourceInfo>::const_iterator& storage_it,
        const rhi::DescriptorReflection* reflection);

    InstanceBufferState* get_instance_buffer(Material* material, const std::string& name);
    bool ensure_buffer_capacity(InstanceBufferState& state, std::size_t required);

    std::size_t gather_matrices_fast(
        const MeshBucket& bucket,
        std::vector<std::uint8_t>& payload);

    std::size_t gather_instance_data(
        const MeshBucket& bucket,
        const std::span<const std::uint8_t>& push_staging,
        std::size_t data_base_offset,
        std::size_t base_copy_size,
        bool can_write_model,
        std::size_t model_offset_in_data,
        const PushConstantRangeRuntime& range,
        std::vector<std::uint8_t>& payload);

    bool upload_to_instance_buffer(
        Material* material,
        const std::string& ssbo_name,
        std::size_t actual_payload_size);

    const PushConstantRangeRuntime* select_fallback_range(
        Material& material,
        const MaterialRenderInfo* render_info);

    void bind_fallback_ssbo(
        Material& material,
        const PushConstantRangeRuntime& range);
}
