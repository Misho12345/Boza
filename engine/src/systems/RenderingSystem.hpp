#pragma once
#include "boza/GraphicsApi.hpp"
#include "boza/platform/Window.hpp"
#include "boza/rhi/Factory.hpp"
#include "boza/rhi/PipelineBuilder.hpp"
#include <chrono>

namespace boza
{
    class RenderingSystem
    {
    public:
        bool init(GraphicsApi api, Window& window);
        void run() const;
        void destroy();

    private:
        std::unique_ptr<rhi::Instance> instance{ nullptr };
        std::unique_ptr<rhi::Device> device{ nullptr };
        std::unique_ptr<rhi::Swapchain> swapchain{ nullptr };
        std::unique_ptr<rhi::ShaderModule> vertex_shader{ nullptr };
        std::unique_ptr<rhi::ShaderModule> fragment_shader{ nullptr };
        std::unique_ptr<rhi::PipelineLayout> pipeline_layout{ nullptr };
        std::unique_ptr<rhi::GraphicsPipeline> graphics_pipeline{ nullptr };
        std::unique_ptr<rhi::Buffer> vertex_buffer{ nullptr };

        // Descriptor resources
        std::unique_ptr<rhi::DescriptorPool> descriptor_pool{ nullptr };
        std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;
        std::vector<rhi::DescriptorSet*> descriptor_sets;

        // Uniform buffers
        std::unique_ptr<rhi::Buffer> ubo1_buffer{ nullptr }; // offset
        std::unique_ptr<rhi::Buffer> ubo2_buffer{ nullptr }; // scale

        // Texture and sampler
        std::unique_ptr<rhi::Texture> texture{ nullptr };
        std::unique_ptr<rhi::Sampler> sampler{ nullptr };

        // Resource binder
        std::unique_ptr<rhi::PipelineResourceBinder> resource_binder{ nullptr };

        mutable std::chrono::steady_clock::time_point start_time;
    };
}
