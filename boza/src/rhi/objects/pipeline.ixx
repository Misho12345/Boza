export module boza.rhi.objects:pipeline;

import std;
import :graphics_object;
import :descriptor;
import :command;
import :resources;
import :shader_module;

export namespace boza::rhi
{
    /// ---------------------------
    /// ===== Pipeline layout =====
    /// ---------------------------

    struct PushConstantRange
    {
        ShaderStage   stage;
        std::uint32_t offset;
        std::uint32_t size;
    };

    struct PipelineLayoutDesc
    {
        Device*                           device;
        std::vector<ShaderModule*>        shaders;
        std::vector<DescriptorSetLayout*> set_layouts;
    };

    class PipelineLayout : public GraphicsObject<PipelineLayout, PipelineLayoutDesc>
    {
    public:
        // template<typename T>
        // void set_push_constant(CommandBuffer* cmd_buffer, const std::string& name, const T& data);
        //
        // template<typename T>
        // void set_push_constant(CommandBuffer* cmd_buffer, const T& data);

    protected:
        explicit PipelineLayout(const PipelineLayoutDesc& desc) : GraphicsObject(desc) {}
    };

    /// -----------------------------
    /// ===== Graphics Pipeline =====
    /// -----------------------------

    enum class PrimitiveTopology : std::uint8_t
    {
        TriangleList,
        TriangleStrip,
        LineList,
        PointList
    };

    enum class PolygonMode : std::uint8_t
    {
        Fill,
        Line,
        Point
    };

    enum class CullMode : std::uint8_t
    {
        None,
        Front,
        Back,
        FrontAndBack
    };

    enum class FrontFace : std::uint8_t
    {
        CounterClockwise,
        Clockwise
    };

    enum class CompareOp : std::uint8_t
    {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum class BlendFactor : std::uint8_t
    {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate
    };

    enum class BlendOp : std::uint8_t
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    struct VertexInputAttribute
    {
        std::uint32_t location;
        std::uint32_t binding;
        std::uint32_t offset;
        // Format will be inferred from shader reflection
    };

    struct VertexInputBinding
    {
        std::uint32_t binding;
        std::uint32_t stride;
        bool          per_instance{ false };
    };

    struct RasterizationState
    {
        PolygonMode polygon_mode{ PolygonMode::Fill };
        CullMode    cull_mode{ CullMode::Back };
        FrontFace   front_face{ FrontFace::CounterClockwise };
        float       line_width{ 1.0f };
        bool        depth_clamp_enable{ false };
        bool        depth_bias_enable{ false };
        float       depth_bias_constant{ 0.0f };
        float       depth_bias_clamp{ 0.0f };
        float       depth_bias_slope{ 0.0f };
    };

    struct DepthStencilState
    {
        bool      depth_test_enable{ true };
        bool      depth_write_enable{ true };
        CompareOp depth_compare_op{ CompareOp::Less };
        bool      depth_bounds_test_enable{ false };
        float     min_depth_bounds{ 0.0f };
        float     max_depth_bounds{ 1.0f };
        bool      stencil_test_enable{ false };
    };

    struct ColorBlendAttachment
    {
        bool        blend_enable{ false };
        BlendFactor src_color_blend_factor{ BlendFactor::One };
        BlendFactor dst_color_blend_factor{ BlendFactor::Zero };
        BlendOp     color_blend_op{ BlendOp::Add };
        BlendFactor src_alpha_blend_factor{ BlendFactor::One };
        BlendFactor dst_alpha_blend_factor{ BlendFactor::Zero };
        BlendOp     alpha_blend_op{ BlendOp::Add };
    };

    struct ColorBlendState
    {
        bool                              logic_op_enable{ false };
        std::vector<ColorBlendAttachment> attachments;
        std::array<float, 4>              blend_constants{ 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct GraphicsPipelineDesc
    {
        Device* device;

        std::vector<ShaderModule*> shaders;
        PipelineLayout*            layout;

        PrimitiveTopology                 topology{ PrimitiveTopology::TriangleList };
        std::vector<VertexInputBinding>   bindings;
        std::vector<VertexInputAttribute> attributes;

        RasterizationState rasterization;
        DepthStencilState  depth_stencil;
        ColorBlendState    color_blend;

        // Dynamic rendering format info
        std::vector<std::uint32_t> color_attachment_formats; // VkFormat values
        std::uint32_t              depth_attachment_format{ 0 };
        std::uint32_t              stencil_attachment_format{ 0 };
    };

    class GraphicsPipeline : public GraphicsObject<GraphicsPipeline, GraphicsPipelineDesc>
    {
    protected:
        explicit GraphicsPipeline(const GraphicsPipelineDesc& desc) : GraphicsObject(desc) {}
    };

    /// ----------------------------
    /// ===== Compute Pipeline =====
    /// ----------------------------

    struct ComputePipelineDesc
    {
        Device*         device;
        ShaderModule*   shader;
        PipelineLayout* layout;
    };

    class ComputePipeline : public GraphicsObject<ComputePipeline, ComputePipelineDesc>
    {
    protected:
        explicit ComputePipeline(const ComputePipelineDesc& desc) : GraphicsObject(desc) {}
    };
}
