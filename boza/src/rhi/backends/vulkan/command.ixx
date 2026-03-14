export module boza.rhi.vulkan:command;

import std;
import boza.rhi.objects;

import <vk_all>;

export namespace boza::rhi::vk
{
    class CommandPool final : public rhi::CommandPool
    {
    public:
        ~CommandPool() override { destroy(); }

        bool init() override;
        void destroy() override;

        CommandBuffer*              allocate_command_buffer(bool is_primary = true) override;
        std::vector<CommandBuffer*> allocate_command_buffers(std::uint32_t count, bool is_primary = true) override;

        void free_command_buffer(CommandBuffer* command_buffer) override;
        void free_command_buffers(std::span<CommandBuffer*> command_buffers) override;

        bool reset(bool release_resources = false) override;

        CommandBuffer* begin_single_time_commands() override;
        bool                end_single_time_commands(CommandBuffer* command_buffer) override;

        [[nodiscard]] VkCommandPool vk_command_pool() const;

    private:
        explicit      CommandPool(const CommandPoolDesc& desc) : rhi::CommandPool(desc) {}
        VkCommandPool vk_command_pool_{ nullptr };

        friend GraphicsObject;
    };

    class CommandBuffer final : public rhi::CommandBuffer
    {
    public:
        ~CommandBuffer() override { destroy(); }

        bool init() override;
        void destroy() override;

        bool begin() override;
        bool begin(Flags<CommandBufferUsage> usage_flags) override;
        bool end() override;
        bool reset(bool release_resources = false) override;

        void draw(
            std::uint32_t vertex_count,
            std::uint32_t instance_count = 1,
            std::uint32_t first_vertex   = 0,
            std::uint32_t first_instance = 0) override;

        void draw_indexed(
            std::uint32_t index_count,
            std::uint32_t instance_count = 1,
            std::uint32_t first_index    = 0,
            std::int32_t  vertex_offset  = 0,
            std::uint32_t first_instance = 0) override;

        void dispatch(std::uint32_t group_x, std::uint32_t group_y, std::uint32_t group_z) override;

        void bind_graphics_pipeline(GraphicsPipeline* pipeline) override;
        void bind_compute_pipeline(ComputePipeline* pipeline) override;

        void bind_vertex_buffer(Buffer* buffer, std::uint32_t binding = 0, std::uint64_t offset = 0) override;
        void bind_index_buffer(Buffer* buffer, std::uint64_t offset = 0, IndexType index_type = IndexType::Uint32) override;

        void bind_descriptor_sets(PipelineLayout* layout, std::span<DescriptorSet*> sets, std::uint32_t first_set) override;

        void push_constants(PipelineLayout* layout, ShaderStage stage, std::uint32_t offset, std::uint32_t size, const void* data) override;

        void image_barrier(Texture* texture, ResourceState old_state, ResourceState new_state) override;

        void pipeline_image_barrier(
            VkImage       image,
            VkImageLayout old_layout,
            VkImageLayout new_layout,

            VkPipelineStageFlags2 src_stage_mask,
            VkAccessFlags2        src_access_mask,
            VkPipelineStageFlags2 dst_stage_mask,
            VkAccessFlags2        dst_access_mask,

            VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
            std::uint32_t base_mip_level   = 0,
            std::uint32_t level_count      = 1,
            std::uint32_t base_array_layer = 0,
            std::uint32_t layer_count      = 1) const;

        [[nodiscard]] VkCommandBuffer vk_command_buffer() const;

    private:
        explicit CommandBuffer(const CommandBufferDesc& desc) : rhi::CommandBuffer(desc) {}

        void set_vk_command_buffer(VkCommandBuffer cmd_buffer);
        VkCommandBuffer vk_command_buffer_{ nullptr };
        VkPipelineBindPoint current_pipeline_bind_point_{ VK_PIPELINE_BIND_POINT_GRAPHICS };

        friend class CommandPool;
    };

    class CommandQueue final : public rhi::CommandQueue
    {
    public:
        ~CommandQueue() override { destroy(); }

        using rhi::CommandQueue::submit;
        using rhi::CommandQueue::present;

        bool init() override;
        void destroy() override;

        bool submit(const SubmitInfo& submit_info) override;
        PresentResult present(const PresentInfo& present_info) override;

        bool wait_idle() override;

        [[nodiscard]] VkQueue vk_queue() const;

    private:
        explicit CommandQueue(const CommandQueueDesc& desc) : rhi::CommandQueue(desc) {}

        VkQueue vk_queue_{ nullptr };

        friend GraphicsObject;
    };
}
