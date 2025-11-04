#pragma once
#include "boza/GraphicsApi.hpp"
#include "boza/platform/Window.hpp"
#include "boza/rhi/Factory.hpp"
#include "boza/rhi/PipelineBuilder.hpp"
#include "boza/core/Scene.hpp"
#include "boza/core/SystemProvider.hpp"
#include "MaterialSystem.hpp"
#include <memory>
#include <unordered_map>

namespace boza
{
    class Mesh;
    namespace rhi { class Buffer; }

    class RenderingSystem final : public SystemProvider
    {
    public:
        bool init(Window& window, std::shared_ptr<Scene> scene);
        void run();
        void destroy();

        IMaterialProvider* material_provider() override;

    private:
        struct GpuMesh
        {
            std::unique_ptr<rhi::Buffer> vertex_buffer;
            std::unique_ptr<rhi::Buffer> index_buffer;
        };

        struct ShaderPipeline
        {
            std::string vertex_shader_name;
            std::string fragment_shader_name;

            std::unique_ptr<rhi::ShaderModule> vertex_shader;
            std::unique_ptr<rhi::ShaderModule> fragment_shader;
            std::unique_ptr<rhi::PipelineLayout> pipeline_layout;
            std::unique_ptr<rhi::GraphicsPipeline> graphics_pipeline;
            std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;

            std::vector<std::vector<std::vector<rhi::DescriptorSet*>>> per_frame_descriptor_set_pool;
            std::vector<std::vector<std::unique_ptr<rhi::PipelineResourceBinder>>> per_frame_binder_pool;

            std::vector<uint32_t> current_descriptor_index;

            static constexpr uint32_t max_descriptor_sets_per_frame = 100;

            void reset_frame_allocations(uint32_t frame_index);

            rhi::PipelineResourceBinder* get_binder_for_draw_call(
                uint32_t frame_index,
                rhi::DescriptorPool* pool,
                const std::vector<rhi::ShaderModule*>& shaders);
        };

        GpuMesh*           get_or_create_gpu_mesh(Mesh* mesh);
        ShaderPipeline*    get_or_create_pipeline(const std::string& vertex_shader, const std::string& fragment_shader);
        static std::string get_pipeline_key(const std::string& vertex_shader, const std::string& fragment_shader);

        void setup_camera_buffer();
        void setup_light_buffer();
        void update_camera_buffer() const;
        void update_light_buffer() const;

        std::shared_ptr<Scene> active_scene_ = nullptr;
        GraphicsApi api_{};
        Window* window_ = nullptr;

        std::unique_ptr<rhi::Instance> instance_{ nullptr };
        std::unique_ptr<rhi::Device> device_{ nullptr };
        std::unique_ptr<rhi::Swapchain> swapchain_{ nullptr };

        std::unique_ptr<MaterialSystem> material_system_{ nullptr };

        std::unique_ptr<rhi::DescriptorPool> descriptor_pool_{ nullptr };
        std::unique_ptr<rhi::Buffer> camera_ubo_{ nullptr };
        std::unique_ptr<rhi::Buffer> light_ubo_{ nullptr };

        std::unique_ptr<rhi::Sampler> sampler_{ nullptr };

        std::unordered_map<Mesh*, GpuMesh> gpu_meshes_;

        std::unordered_map<std::string, std::unique_ptr<ShaderPipeline>> shader_pipelines_;
    };
}
