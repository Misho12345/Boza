export module boza.rhi:pipeline_builder;

import std;
import boza.rhi.api;
import boza.rhi.objects;
import boza.gfx.common;
import boza.common;

export namespace boza::rhi
{
    class PipelineBuilder final
    {
    public:
        PipelineBuilder(GraphicsApi api, Device* device, std::vector<ShaderModule*> shaders);

        bool build_descriptor_set_layouts();

        [[nodiscard]]
        std::unique_ptr<PipelineLayout> build_pipeline_layout() const;

        [[nodiscard]]
        std::unique_ptr<GraphicsPipeline> build_graphics_pipeline(
            PipelineLayout* pipeline_layout,
            const std::vector<TextureFormat>& color_attachment_formats,
            DepthFormat depth_attachment_format = DepthFormat::None,
            const RasterizationState& rasterization = {},
            const DepthStencilState& depth_stencil = {},
            const ColorBlendState& color_blend = {},
            PrimitiveTopology topology = PrimitiveTopology::TriangleList) const;

        [[nodiscard]]
        std::unique_ptr<GraphicsPipeline> build_graphics_pipeline(
            PipelineLayout* pipeline_layout,
            const Swapchain* swapchain,
            DepthFormat depth_attachment_format = DepthFormat::None,
            const RasterizationState& rasterization = {},
            const DepthStencilState& depth_stencil = {},
            const ColorBlendState& color_blend = {},
            PrimitiveTopology topology = PrimitiveTopology::TriangleList) const;

        [[nodiscard]]
        std::unique_ptr<ComputePipeline> build_compute_pipeline(PipelineLayout* pipeline_layout) const;

        [[nodiscard]]
        std::vector<std::unique_ptr<DescriptorSetLayout>> take_descriptor_set_layouts();

    private:
        GraphicsApi api_;
        Device* device_;
        std::vector<ShaderModule*> shaders_;
        std::vector<std::unique_ptr<DescriptorSetLayout>> descriptor_set_layouts_;

        [[nodiscard]] std::vector<DescriptorSetLayout*> get_descriptor_set_layouts() const;

        struct DescriptorBinding final
        {
            std::uint32_t binding;
            DescriptorType type;
            Flags<ShaderStage> stages;
            std::uint32_t count;
        };

        bool merge_descriptor_bindings(flat_map<std::uint32_t, std::vector<DescriptorBinding>>& bindings_by_set) const;
    };
}
