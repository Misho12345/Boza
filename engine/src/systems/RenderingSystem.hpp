#pragma once
#include "boza/GraphicsApi.hpp"
#include "boza/platform/Window.hpp"
#include "boza/rhi/Factory.hpp"

namespace boza
{
    class RenderingSystem
    {
    public:
        bool init(GraphicsApi api, Window& window);
        void run() const;
        void destroy() const;

    private:
        std::unique_ptr<rhi::Instance> instance{ nullptr };
        std::unique_ptr<rhi::Device> device{ nullptr };
        std::unique_ptr<rhi::Swapchain> swapchain{ nullptr };
        std::unique_ptr<rhi::ShaderModule> vertex_shader{ nullptr };
        std::unique_ptr<rhi::ShaderModule> fragment_shader{ nullptr };
        std::unique_ptr<rhi::PipelineLayout> pipeline_layout{ nullptr };
        std::unique_ptr<rhi::GraphicsPipeline> graphics_pipeline{ nullptr };
    };
}
