export module boza.rhi:pipeline_builder;

import std;
import boza.rhi.api;
import boza.rhi.objects;
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
            const std::vector<std::uint32_t>& color_attachment_formats,
            std::uint32_t                     depth_attachment_format = 0,
            const RasterizationState&         rasterization           = {},
            const DepthStencilState&          depth_stencil           = {},
            const ColorBlendState&            color_blend             = {},
            PrimitiveTopology                 topology                = PrimitiveTopology::TriangleList) const;

        GraphicsPipeline* build_graphics_pipeline(
            const Swapchain*          swapchain,
            std::uint32_t             depth_attachment_format = 0,
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

        struct DescriptorBinding
        {
            std::uint32_t      binding;
            DescriptorType     type;
            Flags<ShaderStage> stages;
            std::uint32_t      count;
        };

        std::unordered_map<std::uint32_t, std::vector<DescriptorBinding>> merge_descriptor_bindings() const;
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
            std::uint32_t      offset = 0,
            std::uint32_t      range  = 0) const;
        bool update_storage_buffer(
            const std::string& name,
            Buffer*            buffer,
            std::uint32_t      offset = 0,
            std::uint32_t      range  = 0) const;
        bool update_sampler(const std::string& name, Texture* texture, Sampler* sampler) const;

        void bind_descriptor_sets(CommandBuffer* cmd) const;

        [[nodiscard]] std::uint32_t get_vertex_stride() const;

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
            std::uint32_t  set;
            std::uint32_t  binding;
            DescriptorType type;
            ShaderDataType data_type;
            std::uint32_t  size;
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

     class ComputeDispatcher
    {
    public:
        ComputeDispatcher(
            ShaderModule*                      shader,
            PipelineLayout*                    layout,
            const std::vector<DescriptorSet*>& descriptor_sets);

        class ResourceBinding
        {
        public:
            ResourceBinding(ComputeDispatcher* dispatcher, std::string  name)
                : dispatcher_(dispatcher), name_(std::move(name)) {}

            ResourceBinding& operator=(Texture* texture);
            ResourceBinding& operator=(Buffer* buffer);
            ResourceBinding& operator=(std::pair<Texture*, Sampler*> texture_sampler);

        private:
            ComputeDispatcher* dispatcher_;
            std::string        name_{};
        };

        ResourceBinding operator[](const std::string& name) { return { this, name }; }

        template<typename T>
        bool set_push_constant(CommandBuffer* cmd, const std::string& name, const T& value);
        void bind_and_dispatch(CommandBuffer* cmd, std::uint32_t group_count_x, std::uint32_t group_count_y = 1, std::uint32_t group_count_z = 1) const;
        void dispatch_by_size(CommandBuffer* cmd, std::uint32_t width, std::uint32_t height = 1, std::uint32_t depth = 1) const;

        [[nodiscard]] glm::uvec3 work_group_size() const { return work_group_size_; }

    private:
        friend class ResourceBinding;

        struct ResourceInfo
        {
            std::uint32_t  set;
            std::uint32_t  binding;
            DescriptorType type;
            ShaderDataType data_type;
            std::string    format; // For storage images
            std::string    access; // For storage images (readonly/writeonly/readwrite)
        };

        ShaderModule*               shader_;
        PipelineLayout*             layout_;
        std::vector<DescriptorSet*> descriptor_sets_;
        glm::uvec3                  work_group_size_{ 1, 1, 1 };

        std::unordered_map<std::string, ResourceInfo>                 resource_map_;
        std::unordered_map<std::string, ShaderModule::PushConstant>   push_constant_map_;

        void build_resource_maps();
        bool update_storage_image(const std::string& name, Texture* texture);
        bool update_storage_buffer(const std::string& name, Buffer* buffer);
        bool update_uniform_buffer(const std::string& name, Buffer* buffer);
        bool update_sampler(const std::string& name, Texture* texture, Sampler* sampler);
    };
}
