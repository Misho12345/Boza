module boza.gfx:rendering_system_common;

import std;
import <flecs.h>;
import boza.common;
import boza.core;
import boza.rhi;
import :compute_dispatcher;
import :gpu_driven_instances;

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

        bool has_shadow_view_projection_field{ false };
        std::uint32_t shadow_view_projection_offset_in_range{ 0 };

        bool has_render_mode_field{ false };
        std::uint32_t render_mode_offset_in_range{ 0 };

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

    struct GpuCullInstance
    {
        glm::mat4 model{ 1.0f };
        glm::vec4 sphere{ 0.0f };
    };

    struct GpuCullParams
    {
        std::array<glm::vec4, 6> frustum_planes{};
        glm::uvec4 counts{ 0u };
    };

    struct GpuIndexedDrawCommand
    {
        std::uint32_t index_count{ 0 };
        std::uint32_t instance_count{ 0 };
        std::uint32_t first_index{ 0 };
        std::int32_t vertex_offset{ 0 };
        std::uint32_t first_instance{ 0 };
    };

    struct GpuCullBufferState
    {
        std::unique_ptr<Buffer> params_buffer{};
        std::unique_ptr<Buffer> candidate_buffer{};
        std::unique_ptr<Buffer> culled_instance_buffer{};
        std::unique_ptr<Buffer> indirect_buffer{};
        std::vector<std::unique_ptr<ComputeDispatcher>> dispatchers{};
        std::vector<GameObject> cached_entities{};
        std::vector<GpuCullInstance> cached_candidates{};

        std::size_t params_capacity_bytes{ 0 };
        std::size_t candidate_capacity_bytes{ 0 };
        std::size_t culled_capacity_bytes{ 0 };
        std::size_t indirect_capacity_bytes{ 0 };

        void* bound_culled_handle{ nullptr };
        void* uploaded_candidate_handle{ nullptr };
    };

    struct ShadowCullBufferState
    {
        std::unique_ptr<Buffer> params_buffer{};
        std::unique_ptr<Buffer> candidate_buffer{};
        std::unique_ptr<Buffer> culled_instance_buffer{};
        std::unique_ptr<Buffer> indirect_buffer{};
        std::vector<std::unique_ptr<ComputeDispatcher>> dispatchers{};
        std::vector<GameObject> cached_entities{};
        std::vector<GpuCullInstance> cached_candidates{};
        std::uint64_t cache_frame_{ std::numeric_limits<std::uint64_t>::max() };

        std::size_t params_capacity_bytes{ 0 };
        std::size_t candidate_capacity_bytes{ 0 };
        std::size_t culled_capacity_bytes{ 0 };
        std::size_t indirect_capacity_bytes{ 0 };

        void* bound_culled_handle{ nullptr };
        void* uploaded_candidate_handle{ nullptr };
    };

    struct GpuDrivenBatchState
    {
        std::unique_ptr<Buffer> params_buffer{};
        std::unique_ptr<Buffer> visible_buffer{};
        std::unique_ptr<Buffer> indirect_buffer{};

        std::size_t params_capacity_bytes{ 0 };
        std::size_t visible_capacity_bytes{ 0 };
        std::size_t indirect_capacity_bytes{ 0 };

        void* bound_visible_handle{ nullptr };
    };

    struct GpuDrivenBatchView
    {
        std::uint64_t entity_id{ 0 };
        flecs::entity entity{};
        Mesh* mesh{ nullptr };
        Material* material{ nullptr };
        GpuDrivenInstances* instances{ nullptr };
    };

    struct DirectionalShadowPushConstants
    {
        glm::mat4 light_view_projection{ 1.0f };
        glm::mat4 model{ 1.0f };
    };

    struct ShadowPipelineState
    {
        std::shared_ptr<rhi::ShaderModule> vertex_shader{};
        std::unique_ptr<rhi::PipelineLayout> pipeline_layout{};
        std::unique_ptr<rhi::GraphicsPipeline> pipeline{};
    };

    struct ShadowDrawBatch
    {
        Material* material{ nullptr };
        Mesh* mesh{ nullptr };
        std::vector<glm::mat4> models{};
    };

    struct ShadowCasterGeometry
    {
        glm::vec3 center{ 0.0f };
        glm::vec3 axis_x{ 1.0f, 0.0f, 0.0f };
        glm::vec3 axis_y{ 0.0f, 1.0f, 0.0f };
        glm::vec3 axis_z{ 0.0f, 0.0f, 1.0f };
        glm::vec3 half_extents{ 0.0f };
        glm::vec4 sphere{ 0.0f };
    };

    flat_map<Material*, MaterialRenderInfo> material_render_infos_{};
    flat_map<Material*, flat_map<std::string, InstanceBufferState>> instance_buffers_{};
    flat_map<Material*, flat_map<Mesh*, GpuCullBufferState>> gpu_cull_buffers_{};
    flat_map<Material*, flat_map<Mesh*, ShadowCullBufferState>> shadow_gpu_cull_buffers_{};
    flat_map<Material*, ShadowPipelineState> shadow_pipelines_{};
    flat_map<std::uint64_t, GpuDrivenBatchState> gpu_driven_batch_states_{};

    std::unique_ptr<Buffer> shadow_camera_buffer_{};
    bool directional_shadow_map_layout_initialized_{ false };
    bool point_shadow_map_layout_initialized_{ false };
    bool spot_shadow_map_layout_initialized_{ false };
    std::vector<std::vector<ShadowDrawBatch>> directional_shadow_draws_{};
    std::vector<std::vector<ShadowDrawBatch>> point_shadow_draws_{};
    std::vector<std::vector<ShadowDrawBatch>> spot_shadow_draws_{};

    std::vector<std::unique_ptr<ComputeDispatcher>> cluster_build_dispatchers_{};
    std::vector<std::unique_ptr<ComputeDispatcher>> light_cull_dispatchers_{};
    std::vector<std::unique_ptr<ComputeDispatcher>> ssao_dispatchers_{};
    std::vector<std::unique_ptr<ComputeDispatcher>> gpu_driven_forward_cull_dispatchers_{};
    std::vector<std::unique_ptr<ComputeDispatcher>> gpu_driven_shadow_cull_dispatchers_{};
    std::array<glm::vec4, 6> gpu_cull_frustum_planes_{};

    std::vector<std::uint8_t> push_constant_scratch_{};
    std::vector<std::uint8_t> instance_payload_scratch_{};
    std::vector<GpuCullInstance> gpu_cull_instance_scratch_{};
    std::vector<glm::mat4> shadow_instance_models_{};

    constexpr std::size_t initial_instance_buffer_bytes_ = 65'536 * sizeof(glm::mat4);
    constexpr std::size_t initial_gpu_cull_buffer_bytes_ = 65'536 * sizeof(GpuCullInstance);
    constexpr std::uint32_t shadow_dispatcher_pass_count_ = 3u;
    constexpr std::uint32_t max_shadow_dispatcher_layers_ = 32u;
    constexpr bool gpu_indirect_instancing_enabled_ = false;
    constexpr bool gpu_shadow_instancing_enabled_ = false;
    constexpr std::string_view model_field_name_ = "model";
    constexpr std::string_view instancing_field_name_ = "use_instancing";
    constexpr std::string_view shadow_view_projection_field_name_ = "shadow_view_projection";
    constexpr std::string_view render_mode_field_name_ = "render_mode";

    void assert_render_thread();
    void clear_render_thread();
    [[nodiscard]] std::uint32_t active_frame_index();
    [[nodiscard]] std::uint32_t active_frame_slot_count();
    ComputeDispatcher* get_frame_dispatcher(
        std::vector<std::unique_ptr<ComputeDispatcher>>& dispatchers,
        std::string_view shader_name);

    [[nodiscard]] bool build_shadow_caster_geometry(
        const Transform& transform,
        const ShadowCaster& caster,
        const Mesh* mesh,
        ShadowCasterGeometry& geometry);
    bool refresh_render_element_candidate(
        RenderElement& element,
        Mesh* mesh,
        Material* material);
    bool ensure_mesh_bucket_candidate_buffers(
        MeshBucket& bucket,
        std::size_t forward_candidate_count,
        std::size_t shadow_candidate_count);
    bool upload_mesh_bucket_candidates(
        MeshBucket& bucket,
        Mesh* mesh,
        Material* material);

    void reset_render_caches();
    void clear_shadow_cull_results();

    bool ensure_shadow_camera_buffer();
    bool ensure_shadow_pipeline(Material* material);

    MaterialRenderInfo build_material_render_info(Material* material);
    MaterialRenderInfo* get_or_build_render_info(Material* material);

    void push_ranges(
        rhi::CommandBuffer* cmd,
        Material& material,
        const MaterialRenderInfo* render_info,
        const glm::mat4* model,
        bool use_instancing,
        const glm::mat4* shadow_view_projection = nullptr,
        bool shadow_mode = false);

    bool upload_instance_payload(
        Material* material,
        const MeshBucket& bucket,
        const PushConstantRangeRuntime& range);

    bool upload_instance_payload_from_models(
        Material* material,
        std::span<const glm::mat4> models,
        const PushConstantRangeRuntime& range);

    bool prepare_gpu_culled_instance_payload(
        Material* material,
        Mesh* mesh,
        const MeshBucket& bucket,
        const PushConstantRangeRuntime& range,
        GpuMesh* gpu_mesh,
        Buffer& output_buffer,
        std::uint32_t first_instance);

    [[nodiscard]] bool supports_gpu_culled_instancing(const PushConstantRangeRuntime& range) noexcept;

    [[nodiscard]] GpuCullBufferState* get_gpu_cull_buffer_state(Material* material, Mesh* mesh);
    [[nodiscard]] ShadowCullBufferState* get_shadow_gpu_cull_buffer_state(Material* material, Mesh* mesh);
    bool ensure_generic_gpu_cull_capacity(GpuCullBufferState& state, std::size_t candidate_count);
    bool ensure_generic_shadow_gpu_cull_capacity(ShadowCullBufferState& state, std::size_t candidate_count);
    [[nodiscard]] GpuDrivenBatchState* get_gpu_driven_batch_state(std::uint64_t entity_id);
    void collect_gpu_driven_batches(std::vector<GpuDrivenBatchView>& batches);
    bool ensure_gpu_driven_batch_capacity(
        GpuDrivenBatchState& state,
        std::size_t required_visible_bytes);
    bool dispatch_gpu_driven_cull(
        const Buffer& candidates,
        std::uint32_t candidate_count,
        GpuMesh& gpu_mesh,
        Buffer& params_buffer,
        Buffer& visible_buffer,
        Buffer& indirect_buffer,
        const std::array<glm::vec4, 6>& frustum_planes,
        ComputeDispatcher& dispatcher);
    bool prepare_gpu_shadow_culled_instance_payload(
        Material* material,
        Mesh* mesh,
        const MeshBucket& bucket,
        const PushConstantRangeRuntime& range,
        GpuMesh* gpu_mesh,
        std::uint32_t layer_index,
        std::uint32_t dispatcher_pass_index,
        const std::array<glm::vec4, 6>& frustum_planes,
        Buffer& output_buffer,
        std::uint32_t first_instance);

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

    void bind_frame_render_resources(Material& material);
}
