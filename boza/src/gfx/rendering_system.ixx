export module boza.gfx.rendering_system;

import std;
import boza.common;
import boza.core;
import boza.rhi;
import boza.platform;
import boza.ecs;

export namespace boza::gfx
{
    struct GpuMesh
    {
        std::unique_ptr<rhi::Buffer> vertex_buffer;
        std::unique_ptr<rhi::Buffer> index_buffer;
        std::uint32_t                index_count;
    };

    class RenderingSystem final
    {
    public:
        bool init(platform::Window& window);
        void run();
        void destroy();
        void wait_idle() const;

        void set_active_scene(Scene* scene);
        void set_primary_camera(Camera* camera);

    private:
        void setup_resources();
        void update_camera_uniforms() const;

        GpuMesh* get_or_create_gpu_mesh(Mesh* mesh);

        Camera*                primary_camera_{ nullptr };
        Scene*                 active_scene_{ nullptr };
        rhi::GraphicsApi       api_{};
        platform::Window*      window_{ nullptr };

        std::unique_ptr<rhi::Instance>       instance_{ nullptr };
        std::unique_ptr<rhi::Device>         device_{ nullptr };
        std::unique_ptr<rhi::Swapchain>      swapchain_{ nullptr };
        std::unique_ptr<rhi::DescriptorPool> descriptor_pool_{ nullptr };
        std::unique_ptr<rhi::ResourceCache>  resource_cache_{ nullptr };

        node_map<Mesh*, GpuMesh> gpu_meshes_;

        bool resources_initialized_{ false };
    };
}
