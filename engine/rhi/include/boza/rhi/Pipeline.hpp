#pragma once
#include "boza/std_pch.hpp"
#include "GraphicsObject.hpp"

#include "ShaderModule.hpp"
#include "DescriptorSet.hpp"
#include "Command.hpp"

namespace boza::rhi
{
    /// ---------------------------
    /// ===== Pipeline layout =====
    /// ---------------------------

    struct PushConstantRange
    {
        ShaderStage stage;
        uint32_t    offset;
        uint32_t    size;
    };

    struct PipelineLayoutDesc
    {
        Device*                           device;
        std::vector<DescriptorSetLayout*> set_layouts;
        std::vector<PushConstantRange>    push_constant_ranges;
    };

    class PipelineLayout : public GraphicsObject<PipelineLayout, PipelineLayoutDesc>
    {
    protected:
        explicit PipelineLayout(const PipelineLayoutDesc& desc) : GraphicsObject(desc) {}
    };

    /// -----------------------------
    /// ===== Graphics Pipeline =====
    /// -----------------------------

    enum class PrimitiveTopology : uint8_t
    {
        TriangleList,
        TriangleStrip,
        LineList,
        PointList
    };

    struct VertexInputAttribute
    {
        uint32_t location;
    };

    struct VertexInputBinding
    {
        uint32_t binding;
        uint32_t stride;
    };

    struct GraphicsPipelineDesc
    {
        Device* device;

        ShaderModule*   vertex_shader;
        ShaderModule*   fragment_shader;
        PipelineLayout* layout;

        PrimitiveTopology                 topology;
        std::vector<VertexInputBinding>   bindings;
        std::vector<VertexInputAttribute> attributes;
    };

    class GraphicsPipeline : public GraphicsObject<GraphicsPipeline, GraphicsPipelineDesc>
    {
    public:
        virtual void bind(CommandBuffer* cmd_buffer) = 0;

    protected:
        explicit GraphicsPipeline(const GraphicsPipelineDesc& desc) : GraphicsObject(desc) {}
    };

    /// ----------------------------
    /// ===== Compute Pipeline =====
    /// ----------------------------

    struct ComputePipelineDesc
    {
        PipelineLayout* layout;
    };

    class ComputePipeline : public GraphicsObject<ComputePipeline, ComputePipelineDesc>
    {
    public:
        virtual void bind(CommandBuffer* cmd_buffer) = 0;

    protected:
        explicit ComputePipeline(const ComputePipelineDesc& desc) : GraphicsObject(desc) {}
    };
}
