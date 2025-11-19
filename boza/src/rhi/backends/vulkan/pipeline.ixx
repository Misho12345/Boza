export module boza.rhi.vulkan:pipeline;

import std;
import boza.rhi.objects;

import <vk_all.h>;

export namespace boza::rhi::vk
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
        explicit   GraphicsPipeline(const GraphicsPipelineDesc& desc) : rhi::GraphicsPipeline(desc) {}
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
        explicit   ComputePipeline(const ComputePipelineDesc& desc) : rhi::ComputePipeline(desc) {}
        VkPipeline vk_pipeline_{ nullptr };

        friend GraphicsObject;
    };
}

namespace boza::rhi::vk
{
    VkShaderStageFlagBits to_vk(const ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
            case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
            case ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
            case ShaderStage::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case ShaderStage::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
            default: return VK_SHADER_STAGE_ALL;
        }
    }

    VkPrimitiveTopology to_vk(const PrimitiveTopology topology)
    {
        switch (topology)
        {
            case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    VkPolygonMode to_vk(const PolygonMode mode)
    {
        switch (mode)
        {
            case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
            case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
            case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
            default: return VK_POLYGON_MODE_FILL;
        }
    }

    VkCullModeFlags to_vk(const CullMode mode)
    {
        switch (mode)
        {
            case CullMode::None: return VK_CULL_MODE_NONE;
            case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
            case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
            case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
            default: return VK_CULL_MODE_BACK_BIT;
        }
    }

    VkFrontFace to_vk(const FrontFace face)
    {
        switch (face)
        {
            case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
            default: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }
    }

    VkCompareOp to_vk(const CompareOp op)
    {
        switch (op)
        {
            case CompareOp::Never: return VK_COMPARE_OP_NEVER;
            case CompareOp::Less: return VK_COMPARE_OP_LESS;
            case CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
            case CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
            case CompareOp::Greater: return VK_COMPARE_OP_GREATER;
            case CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
            case CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case CompareOp::Always: return VK_COMPARE_OP_ALWAYS;
            default: return VK_COMPARE_OP_LESS;
        }
    }

    VkBlendFactor to_vk(const BlendFactor factor)
    {
        switch (factor)
        {
            case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
            case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
            case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
            case BlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
            case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            case BlendFactor::ConstantColor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
            case BlendFactor::OneMinusConstantColor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
            case BlendFactor::ConstantAlpha: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
            case BlendFactor::OneMinusConstantAlpha: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
            case BlendFactor::SrcAlphaSaturate: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
            default: return VK_BLEND_FACTOR_ZERO;
        }
    }

    VkBlendOp to_vk(const BlendOp op)
    {
        switch (op)
        {
            case BlendOp::Add: return VK_BLEND_OP_ADD;
            case BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
            case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
            case BlendOp::Min: return VK_BLEND_OP_MIN;
            case BlendOp::Max: return VK_BLEND_OP_MAX;
            default: return VK_BLEND_OP_ADD;
        }
    }

    VkFormat to_vk(const ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float: return VK_FORMAT_R32_SFLOAT;
            case ShaderDataType::Vec2: return VK_FORMAT_R32G32_SFLOAT;
            case ShaderDataType::Vec3: return VK_FORMAT_R32G32B32_SFLOAT;
            case ShaderDataType::Vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case ShaderDataType::Int: return VK_FORMAT_R32_SINT;
            case ShaderDataType::IVec2: return VK_FORMAT_R32G32_SINT;
            case ShaderDataType::IVec3: return VK_FORMAT_R32G32B32_SINT;
            case ShaderDataType::IVec4: return VK_FORMAT_R32G32B32A32_SINT;
            case ShaderDataType::Uint: return VK_FORMAT_R32_UINT;
            case ShaderDataType::UVec2: return VK_FORMAT_R32G32_UINT;
            case ShaderDataType::UVec3: return VK_FORMAT_R32G32B32_UINT;
            case ShaderDataType::UVec4: return VK_FORMAT_R32G32B32A32_UINT;
            case ShaderDataType::Double: return VK_FORMAT_R64_SFLOAT;
            default: return VK_FORMAT_R32G32B32_SFLOAT;
        }
    }
}
