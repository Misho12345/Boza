export module boza.rhi:pipeline_builder;

import std;
import boza.rhi.api;
import boza.rhi.objects;
import boza.gfx;
import boza.common;

export namespace boza::rhi
{
    class PipelineBuilder final
    {
    public:
        PipelineBuilder(GraphicsApi api, Device* device, const std::vector<ShaderModule*>& shaders);

        bool build_descriptor_set_layouts();

        PipelineLayout* build_pipeline_layout();

        GraphicsPipeline* build_graphics_pipeline(
            const std::vector<TextureFormat>& color_attachment_formats,
            DepthFormat                       depth_attachment_format = DepthFormat::None,
            const RasterizationState&         rasterization           = {},
            const DepthStencilState&          depth_stencil           = {},
            const ColorBlendState&            color_blend             = {},
            PrimitiveTopology                 topology                = PrimitiveTopology::TriangleList) const;

        GraphicsPipeline* build_graphics_pipeline(
            const Swapchain*          swapchain,
            DepthFormat               depth_attachment_format = DepthFormat::None,
            const RasterizationState& rasterization           = {},
            const DepthStencilState&  depth_stencil           = {},
            const ColorBlendState&    color_blend             = {},
            PrimitiveTopology         topology                = PrimitiveTopology::TriangleList) const;

        [[nodiscard]]
        ComputePipeline* build_compute_pipeline() const;

        [[nodiscard]] const std::vector<DescriptorSetLayout*>& get_descriptor_set_layouts() const;

    private:
        GraphicsApi                       api_;
        Device*                           device_;
        std::vector<ShaderModule*>        shaders_;
        std::vector<DescriptorSetLayout*> descriptor_set_layouts_;
        PipelineLayout*                   pipeline_layout_{ nullptr };

        struct DescriptorBinding final
        {
            std::uint32_t      binding;
            DescriptorType     type;
            Flags<ShaderStage> stages;
            std::uint32_t      count;
        };

        flat_map<std::uint32_t, std::vector<DescriptorBinding>> merge_descriptor_bindings() const;
    };
}
