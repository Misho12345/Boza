export module boza.gfx:rendering_system;

import std;
import boza.common;
import boza.core;
import boza.ecs;
import boza.input;

import :mesh;
import :buffer;
import :mesh_renderer;

namespace boza
{
    class App;

    namespace rhi
    {
        class Instance;
        class Device;
        class Swapchain;
        class DescriptorPool;
        class ResourceCache;
        class GraphicsPipeline;
        class CommandBuffer;
        class PipelineLayout;
    }

    struct GpuMesh
    {
        Buffer vertex_buffer;
        Buffer index_buffer;
        std::uint32_t index_count;
    };

    struct RenderElement
    {
        GameObject entity{};
        bool visible{ false };
    };

    struct MeshBucket
    {
        std::vector<RenderElement> elements{};
        std::uint32_t num_visible{ 0 };
    };

    struct MaterialRenderGroup
    {
        flat_map<Mesh*, MeshBucket> mesh_buckets{};
        std::uint32_t num_visible{ 0 };
        bool should_try_instancing{ false };
    };

    export struct RenderingSystem final
    {
        struct EngineBegin : EngineBeginStage<EngineBegin>
        {
            static SystemStageConfig config()
            {
                return {
                    .run_before = { InputSystem::Begin::stage_info.system }
                };
            }

            static void execute();
        };

        struct SanityCheck : EngineRenderStage<SanityCheck>
        {
            static void execute();
        };

        struct ProcessMeshChanged : EngineRenderStage<
            ProcessMeshChanged,
            With<MeshRenderer>,
            With<tags::MeshChanged>>
        {
            static SystemStageConfig config()
            {
                return {
                    .run_after = { SanityCheck::stage_info.system }
                };
            }

            static void execute(GameObject go, MeshRenderer& mr);
        };

        struct ProcessMaterialChanged : EngineRenderStage<
            ProcessMaterialChanged,
            With<MeshRenderer>,
            With<tags::MaterialChanged>>
        {
            static SystemStageConfig config()
            {
                return {
                    .run_after = { ProcessMeshChanged::stage_info.system }
                };
            }

            static void execute(GameObject go, MeshRenderer& mr);
        };

        struct ProcessInvalidated : EngineRenderStage<
            ProcessInvalidated,
            With<MeshRenderer>,
            With<tags::RenderCacheInvalidated>>
        {
            static SystemStageConfig config()
            {
                return {
                    .include_disabled = true,
                    .run_after = { ProcessMaterialChanged::stage_info.system }
                };
            }

            static void execute(GameObject go, MeshRenderer& mr);
        };

        struct BeginFrame : EngineRenderStage<BeginFrame>
        {
            static SystemStageConfig config()
            {
                return {
                    .run_after = { ProcessInvalidated::stage_info.system }
                };
            }

            static void execute();
        };

        struct CameraUboUpdate : EngineRenderStage<
            CameraUboUpdate,
            With<const Camera>,
            With<const Transform>,
            With<tags::PrimaryCamera>>
        {
            static SystemStageConfig config()
            {
                return {
                    .run_after = { BeginFrame::stage_info.system }
                };
            }

            static void execute(const Camera& cam, const Transform& transform);
        };

        struct CullEntities : EngineRenderStage<CullEntities>
        {
            static SystemStageConfig config()
            {
                return {
                    .run_after = { CameraUboUpdate::stage_info.system }
                };
            }

            static void execute();
        };

        struct EndFrame : EngineRenderStage<EndFrame>
        {
            static SystemStageConfig config()
            {
                return {
                    .run_after = { CullEntities::stage_info.system }
                };
            }

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
        static void setup_resources();
        static void wait_idle();
        static GpuMesh* get_or_create_gpu_mesh(Mesh* mesh);

        static bool check_mesh_valid(Mesh* mesh);
        static bool check_material_valid(Material* material);
        static void rebuild_pipeline_materials();

        static void remove_from_render_cache(MeshRenderer& mr, GameObject go);
        static void remove_from_unresolved(MeshRenderer& mr, GameObject go);
        static void insert_into_render_cache(MeshRenderer& mr, GameObject go, Mesh* mesh, Material* material);
        static void insert_into_unresolved(MeshRenderer& mr, GameObject go);

        static void clear_validity_caches();
        static void validate_render_cache();
        static void evict_material_group(MaterialRenderGroup& group);
        static void validate_mesh_buckets(MaterialRenderGroup& mat_group);
        static void evict_mesh_bucket(MeshBucket& bucket);
        static void prune_invalid_entities(MeshBucket& bucket);
        static void validate_unresolved_list();
        static void try_resolve_unresolved();

        static void submit_draws();
        static void draw_material_meshes(
            rhi::CommandBuffer* cmd,
            Material* material,
            const MaterialRenderGroup& mat_group,
            void* render_info,
            std::size_t max_instance_index);

        static bool try_instanced_draw(
            rhi::CommandBuffer* cmd,
            Material* material,
            const MeshBucket& bucket,
            void* render_info,
            GpuMesh* gpu_mesh);

        static void draw_elements_individually(
            rhi::CommandBuffer* cmd,
            Material* material,
            const MeshBucket& bucket,
            void* render_info,
            GpuMesh* gpu_mesh);

        static std::unique_ptr<rhi::Instance> instance_;
        static std::unique_ptr<rhi::Device> device_;
        static std::unique_ptr<rhi::Swapchain> swapchain_;
        static std::unique_ptr<rhi::DescriptorPool> descriptor_pool_;
        static std::unique_ptr<rhi::ResourceCache> resource_cache_;

        static inline node_map<Mesh*, GpuMesh> gpu_meshes_{};
        static inline flat_map<Material*, MaterialRenderGroup> render_cache_{};
        static inline std::vector<GameObject> unresolved_{};
        static inline flat_map<rhi::GraphicsPipeline*, std::vector<Material*>> pipeline_materials_{};

        static inline flat_set<Mesh*> valid_meshes_{};
        static inline flat_set<Mesh*> invalid_meshes_{};
        static inline flat_set<Material*> valid_materials_{};
        static inline flat_set<Material*> invalid_materials_{};

        static inline FrustumState frustum_{};
        static inline bool frame_active_{ false };

        static inline flat_set<Mesh*> destroyed_meshes_this_frame_{};
        static inline flat_set<Material*> destroyed_materials_this_frame_{};

        static inline bool pipeline_materials_dirty_{ true };
        static inline std::uint32_t sanity_frame_counter_{ 0 };
        static constexpr std::uint32_t full_sanity_interval_{ 300 };
        static constexpr std::size_t instancing_threshold_ = 16;

        friend class App;
    };
}
