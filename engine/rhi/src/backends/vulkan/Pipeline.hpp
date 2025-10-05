#pragma once
#include "pch.hpp"
#include "boza/rhi/Pipeline.hpp"

namespace boza::rhi::vk
{
    class PipelineLayout final : public rhi::PipelineLayout
    {
    public:
        bool init() override;
        void destroy() override;

        template<typename T>
        void set_push_constant(CommandBuffer* cmd_buffer, const std::string& name, const T& data);

        template<typename T>
        void set_push_constant(CommandBuffer* cmd_buffer, const T& data);

        [[nodiscard]] VkPipelineLayout vk_pipeline_layout() const;

    private:
        explicit PipelineLayout(const PipelineLayoutDesc& desc) : rhi::PipelineLayout(desc) {}
        VkPipelineLayout vk_pipeline_layout_{ nullptr };
        std::unordered_map<std::string, rhi::ShaderModule::PushConstant> push_constants_;

        friend GraphicsObject;
    };

    class GraphicsPipeline final : public rhi::GraphicsPipeline
    {
    public:
        bool init() override;
        void destroy() override;

        [[nodiscard]] VkPipeline vk_pipeline() const;

    private:
        explicit GraphicsPipeline(const GraphicsPipelineDesc& desc) : rhi::GraphicsPipeline(desc) {}
        VkPipeline vk_pipeline_{ nullptr };

        friend GraphicsObject;
    };

    class ComputePipeline final : public rhi::ComputePipeline
    {
    public:
        bool init() override;
        void destroy() override;

        [[nodiscard]] VkPipeline vk_pipeline() const;

    private:
        explicit ComputePipeline(const ComputePipelineDesc& desc) : rhi::ComputePipeline(desc) {}
        VkPipeline vk_pipeline_{ nullptr };

        friend GraphicsObject;
    };
}