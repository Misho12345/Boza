#pragma once
#include "boza/pch.hpp"
#include "Pipeline.hpp"
#include "ShaderModule.hpp"
#include "Descriptor.hpp"
#include "Swapchain.hpp"
#include "boza/GraphicsApi.hpp"

namespace boza::rhi
{
    class PipelineBuilder final
    {
    public:
        PipelineBuilder(GraphicsApi api, Device* device, const std::vector<ShaderModule*>& shaders);

        bool build_descriptor_set_layouts();

        PipelineLayout* build_pipeline_layout();

        GraphicsPipeline* build_graphics_pipeline(
            const std::vector<uint32_t>& color_attachment_formats,
            uint32_t                     depth_attachment_format = 0,
            const RasterizationState&    rasterization           = {},
            const DepthStencilState&     depth_stencil           = {},
            const ColorBlendState&       color_blend             = {},
            PrimitiveTopology            topology                = PrimitiveTopology::TriangleList) const;

        GraphicsPipeline* build_graphics_pipeline(
            const Swapchain*          swapchain,
            uint32_t                  depth_attachment_format = 0,
            const RasterizationState& rasterization           = {},
            const DepthStencilState&  depth_stencil           = {},
            const ColorBlendState&    color_blend             = {},
            PrimitiveTopology         topology                = PrimitiveTopology::TriangleList) const;

        [[nodiscard]] const std::vector<DescriptorSetLayout*>& get_descriptor_set_layouts() const;

    private:
        GraphicsApi                       api_;
        Device*                           device_;
        std::vector<ShaderModule*>        shaders_;
        std::vector<DescriptorSetLayout*> descriptor_set_layouts_;
        PipelineLayout*                   pipeline_layout_{ nullptr };

        struct DescriptorBinding
        {
            uint32_t           binding;
            DescriptorType     type;
            Flags<ShaderStage> stages;
            uint32_t           count;
        };

        std::unordered_map<uint32_t, std::vector<DescriptorBinding>> merge_descriptor_bindings() const;
    };

    class PipelineResourceBinder
    {
    public:
        PipelineResourceBinder(
            const std::vector<ShaderModule*>&  shaders,
            PipelineLayout*                    layout,
            const std::vector<DescriptorSet*>& descriptor_sets);

        template<typename T>
        bool set_push_constant(CommandBuffer* cmd, const std::string& name, const T& value);

        bool bind_vertex_buffer(CommandBuffer* cmd, const std::string& attribute_name, Buffer* buffer) const;

        bool update_uniform_buffer(
            const std::string& name,
            Buffer*            buffer,
            uint32_t           offset = 0,
            uint32_t           range  = 0) const;
        bool update_storage_buffer(const std::string& name, Buffer* buffer, uint32_t offset = 0, uint32_t range = 0) const;
        bool update_sampler(const std::string& name, Texture* texture, Sampler* sampler) const;

        void bind_descriptor_sets(CommandBuffer* cmd) const;

        [[nodiscard]] uint32_t get_vertex_stride() const;

        [[nodiscard]] std::vector<VertexInputAttribute> get_vertex_attributes() const;
        [[nodiscard]] std::vector<VertexInputBinding>   get_vertex_bindings() const;

    private:
        struct PushConstantInfo
        {
            ShaderModule::PushConstant                                        pc;
            std::unordered_map<std::string, ShaderModule::PushConstantMember> members;
        };

        struct ResourceInfo
        {
            uint32_t       set;
            uint32_t       binding;
            DescriptorType type;
            ShaderDataType data_type;
            uint32_t       size;
        };

        std::vector<ShaderModule*>  shaders_;
        PipelineLayout*             layout_;
        std::vector<DescriptorSet*> descriptor_sets_;

        std::unordered_map<std::string, PushConstantInfo>             push_constant_map_;
        std::unordered_map<std::string, ResourceInfo>                 resource_map_;
        std::unordered_map<std::string, ShaderModule::ShaderResource> vertex_inputs_;

        void                        build_resource_maps();
        [[nodiscard]] static size_t get_type_size(ShaderDataType type);
    };
}
