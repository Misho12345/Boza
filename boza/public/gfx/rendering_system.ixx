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
    }

    struct GpuMesh
    {
        Buffer        vertex_buffer;
        Buffer        index_buffer;
        std::uint32_t index_count;
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

        struct BeginFrame : PreRenderStage<BeginFrame>
        {
            static void execute();
        };

        struct EndFrame : EngineRenderStage<EndFrame>
        {
            static void execute();
        };

        struct Render : EngineRenderStage<Render, With<MeshRenderer>, With<const Transform>>
        {
            static SystemStageConfig config()
            {
                return {
                    .run_after = { BeginFrame::stage_info.system },
                    .run_before = { EndFrame::stage_info.system }
                };
            }

            static void execute(MeshRenderer& mr, const Transform& t);
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
                    .run_after = { BeginFrame::stage_info.system },
                    .run_before = { Render::stage_info.system }
                };
            }

            static void execute(const Camera& cam, const Transform& transform);
        };

        struct EngineDestroy : EngineDestroyStage<EngineDestroy>
        {
            static void execute();
        };

    private:
        static bool     init_graphics();
        static void     setup_resources();
        static void     wait_idle();
        static GpuMesh* get_or_create_gpu_mesh(Mesh* mesh);

        static std::unique_ptr<rhi::Instance>       instance_;
        static std::unique_ptr<rhi::Device>         device_;
        static std::unique_ptr<rhi::Swapchain>      swapchain_;
        static std::unique_ptr<rhi::DescriptorPool> descriptor_pool_;
        static std::unique_ptr<rhi::ResourceCache>  resource_cache_;

        static inline node_map<Mesh*, GpuMesh> gpu_meshes_{};

        friend class App;
    };
}
