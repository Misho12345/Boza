export module boza.gfx:rendering_system;

import std;
import boza.common;
import boza.core;
import boza.ecs;
import boza.input;

import :mesh;
import :buffer;
import :mesh_renderer;
import :camera;
import :light;

namespace boza
{
    class App;

    struct GpuMesh
    {
        Buffer vertex_buffer;
        Buffer index_buffer;
        std::uint32_t index_count;
        std::uint64_t mesh_revision{ 0 };
    };

    export struct RenderingSystem final
    {
        struct EngineBegin : EngineBeginStage<EngineBegin, RunBefore<InputSystem::Begin>>
        {
            static void execute();
        };

        struct SanityCheck : EngineRenderStage<SanityCheck>
        {
            static void execute();
        };

        struct ProcessMeshChanged : EngineRenderStage<ProcessMeshChanged,
            RunAfter<SanityCheck>,
            With<MeshRenderer>,
            With<tags::MeshChanged>>
        {
            static void execute(GameObject go, MeshRenderer& mr);
        };

        struct ProcessMaterialChanged : EngineRenderStage<ProcessMaterialChanged,
            RunAfter<ProcessMeshChanged>,
            With<MeshRenderer>,
            With<tags::MaterialChanged>>
        {
            static void execute(GameObject go, MeshRenderer& mr);
        };

        struct BeginFrame : EngineRenderStage<BeginFrame, RunAfter<ProcessMaterialChanged>>
        {
            static void execute();
        };

        struct ProcessTransformChanged : EngineRenderStage<ProcessTransformChanged,
            RunAfter<ProcessMaterialChanged>,
            RunBefore<BeginFrame>,
            With<MeshRenderer>,
            With<tags::RenderTransformDirty>>
        {
            static SystemStageConfig config() { return { .include_disabled = true }; }
            static void execute(GameObject go, MeshRenderer& mr);
        };

        struct CameraUboUpdate : EngineRenderStage<CameraUboUpdate,
            RunAfter<BeginFrame>,
            With<const Camera>,
            With<const Transform>,
            With<tags::PrimaryCamera>>
        {
            static void execute(const Camera& cam, const Transform& transform);
        };

        struct GatherPointLights : EngineRenderStage<GatherPointLights,
            RunAfter<CameraUboUpdate>,
            With<const Transform>,
            With<const PointLight>>
        {
            static void execute(const Transform& transform, const PointLight& light);
        };

        struct GatherDirectionalLights : EngineRenderStage<GatherDirectionalLights,
            RunAfter<CameraUboUpdate>,
            With<const Transform>,
            With<const DirectionalLight>>
        {
            static void execute(const Transform& transform, const DirectionalLight& light);
        };

        struct GatherSpotLights : EngineRenderStage<GatherSpotLights,
            RunAfter<CameraUboUpdate>,
            With<const Transform>,
            With<const SpotLight>>
        {
            static void execute(const Transform& transform, const SpotLight& light);
        };

        struct GatherShadowCasters : EngineRenderStage<GatherShadowCasters,
            RunAfter<CameraUboUpdate>,
            With<const Transform>,
            With<const ShadowCaster>,
            Opt<const MeshRenderer>>
        {
            static void execute(const Transform& transform, const ShadowCaster& caster, const MeshRenderer* renderer);
        };

        struct UploadLightBuffers : EngineRenderStage<UploadLightBuffers,
            RunAfter<GatherPointLights>,
            RunAfter<GatherDirectionalLights>,
            RunAfter<GatherSpotLights>,
            RunAfter<GatherShadowCasters>>
        {
            static void execute();
        };

        struct PrepareGpuCulling : EngineRenderStage<PrepareGpuCulling, RunAfter<UploadLightBuffers>>
        {
            static void execute();
        };

        struct EndFrame : EngineRenderStage<EndFrame, RunAfter<PrepareGpuCulling>>
        {
            static void execute();
        };

        struct EngineDestroy : EngineDestroyStage<EngineDestroy>
        {
            static void execute();
        };

        static void on_mesh_destroyed(Mesh* mesh);
        static void on_material_destroyed(Material* material);

    private:
        struct FrustumPlane
        {
            glm::vec3 normal{ 0.0f };
            float distance{ 0.0f };
        };

        struct FrustumState
        {
            bool valid{ false };
            std::array<FrustumPlane, 6> planes{};

            static FrustumPlane normalize_plane(const glm::vec4& plane);
            void set_from_view_projection(const glm::mat4& vp);
            [[nodiscard]] bool sphere_visible(const glm::vec3& center, float radius) const;
        };

        static bool init_graphics();
        static void clear_runtime_state();
        static void shutdown_graphics(bool wait_for_device, bool destroy_window);
        static void setup_resources();
        static void wait_idle();
        static GpuMesh* get_or_create_gpu_mesh(Mesh* mesh);

        static bool check_mesh_valid(Mesh* mesh);
        static bool check_material_valid(Material* material);
        static void process_renderer_binding_change(GameObject go, MeshRenderer& mr, bool mesh_changed);

        static void clear_validity_caches();

        static void submit_gpu_driven_draws();
        static void prepare_frame_material_bindings();
        static void execute_gpu_instance_culling_pass();
        static void execute_depth_prepass();
        static void execute_shadow_caster_cull_pass();
        static void execute_directional_shadow_pass();
        static void execute_point_shadow_pass();
        static void execute_spot_shadow_pass();
        static bool execute_clustered_light_culling_pass();
        static bool execute_ssao_pass();
        static void execute_forward_pass();

        static bool ensure_shadow_pipeline(Material* material);
        static void bind_shadow_camera(Material& material, const glm::mat4& light_view_projection);
        static void render_shadow_pass(
            Texture* shadow_map,
            std::span<const glm::mat4> matrices,
            bool& layout_initialized,
            std::string_view log_label,
            std::uint32_t dispatcher_pass_index);

        static inline node_map<Mesh*, GpuMesh> gpu_meshes_{};

        static inline flat_set<Mesh*> valid_meshes_{};
        static inline flat_set<Mesh*> invalid_meshes_{};
        static inline flat_set<Material*> valid_materials_{};
        static inline flat_set<Material*> invalid_materials_{};

        static inline FrustumState frustum_{};
        static inline bool frame_active_{ false };

        static inline flat_set<Mesh*> destroyed_meshes_this_frame_{};
        static inline flat_set<Material*> destroyed_materials_this_frame_{};

        static inline std::uint32_t sanity_frame_counter_{ 0 };
        static constexpr std::uint32_t full_sanity_interval_{ 300 };
        static constexpr std::size_t instancing_threshold_ = 16;

        friend class App;
    };
}
