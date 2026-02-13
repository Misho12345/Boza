export module boza.rhi.vulkan:pipeline;

import std;
import boza.common;
import boza.rhi.objects;

import <vk_all>;

export namespace boza::rhi::vk
{
    class PipelineLayout final : public rhi::PipelineLayout
    {
    public:
        ~PipelineLayout() override { destroy(); }

        bool init() override;
        void destroy() override;

        [[nodiscard]] VkPipelineLayout vk_pipeline_layout() const;

    private:
        explicit PipelineLayout(const PipelineLayoutDesc& desc) : rhi::PipelineLayout(desc) {}
        VkPipelineLayout vk_pipeline_layout_{ nullptr };
        flat_map<std::string, ShaderModule::PushConstant> push_constants_;

        friend GraphicsObject;
    };

    class GraphicsPipeline final : public rhi::GraphicsPipeline
    {
    public:
        ~GraphicsPipeline() override { destroy(); }

        bool init() override;
        void destroy() override;

        [[nodiscard]] VkPipeline vk_pipeline() const;

    private:
        explicit   GraphicsPipeline(const GraphicsPipelineDesc& desc) : rhi::GraphicsPipeline(desc) {}
        VkPipeline vk_pipeline_{ nullptr };

        friend GraphicsObject;
    };

    class ComputePipeline final : public rhi::ComputePipeline
    {
    public:
        ~ComputePipeline() override { destroy(); }

        bool init() override;
        void destroy() override;

        [[nodiscard]] VkPipeline vk_pipeline() const;

    private:
        explicit   ComputePipeline(const ComputePipelineDesc& desc) : rhi::ComputePipeline(desc) {}
        VkPipeline vk_pipeline_{ nullptr };

        friend GraphicsObject;
    };
}
