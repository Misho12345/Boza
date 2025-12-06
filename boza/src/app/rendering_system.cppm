module boza.app:rendering_system;

import std;
import boza.ecs;
import boza.gfx;
import boza.rhi;
import boza.platform;
import boza.common;

using boza::platform::Window;

namespace boza::app
{
    struct GpuMesh
    {
        std::unique_ptr<rhi::Buffer> vertex_buffer;
        std::unique_ptr<rhi::Buffer> index_buffer;
        std::uint32_t index_count;
    };

    struct CameraUBO
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct MaterialUBO
    {
        glm::vec4 albedo_color;
        glm::vec4 properties;
    };

    struct LightUBO
    {
        glm::vec4 light_position;
        glm::vec4 light_color;
        glm::vec4 view_pos;
    };

    class RenderingSystem final
    {
    public:
        bool init(Window& window, std::shared_ptr<Scene> scene);
        void run();
        void destroy();

    private:
        void setup_test_resources();
        GpuMesh* get_or_create_gpu_mesh(Mesh* mesh);
        void update_camera_uniforms();

        std::shared_ptr<Scene> active_scene_{ nullptr };
        rhi::GraphicsApi api_{};
        Window* window_ = nullptr;

        std::unique_ptr<rhi::Instance> instance_{ nullptr };
        std::unique_ptr<rhi::Device> device_{ nullptr };
        std::unique_ptr<rhi::Swapchain> swapchain_{ nullptr };
        std::unique_ptr<rhi::DescriptorPool> descriptor_pool_{ nullptr };
        std::unique_ptr<rhi::ResourceCache> resource_cache_{ nullptr };

        std::unordered_map<Mesh*, std::unique_ptr<GpuMesh>> gpu_meshes_;
        std::unordered_map<std::string, Material*> materials_;

        std::unique_ptr<rhi::Buffer> camera_ubo_;
        std::unique_ptr<rhi::Buffer> light_ubo_;
        std::unique_ptr<rhi::Buffer> material_ubo_;
        std::unique_ptr<rhi::Sampler> default_sampler_;
        Texture* default_texture_{ nullptr };

        bool resources_initialized_{ false };
        bool compute_texture_pending_{ false };
        ComputeDispatcher* pending_compute_delete_{ nullptr };
    };
}
