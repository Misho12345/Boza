#pragma once
#include "pch.hpp"
#include "boza/rhi/Command.hpp"
#include "boza/rhi/Descriptor.hpp"

namespace boza::rhi::vk
{
    class CommandPool final : public rhi::CommandPool
    {
    public:
        bool init() override;
        void destroy() override;

        rhi::CommandBuffer*              allocate_command_buffer(bool is_primary = true) override;
        std::vector<rhi::CommandBuffer*> allocate_command_buffers(uint32_t count, bool is_primary = true) override;

        void free_command_buffer(rhi::CommandBuffer* command_buffer) override;
        void free_command_buffers(const std::vector<rhi::CommandBuffer*>& command_buffers) override;

        bool reset(bool release_resources = false) override;

        rhi::CommandBuffer* begin_single_time_commands() override;
        bool                end_single_time_commands(rhi::CommandBuffer* command_buffer) override;

        [[nodiscard]] VkCommandPool vk_command_pool() const;

    private:
        explicit      CommandPool(const CommandPoolDesc& desc) : rhi::CommandPool(desc) {}
        VkCommandPool vk_command_pool_{ nullptr };

        friend GraphicsObject;
    };

    class CommandBuffer final : public rhi::CommandBuffer
    {
    public:
        bool init() override;
        void destroy() override;

        bool begin() override;
        bool begin(Flags<CommandBufferUsage> usage_flags) override;
        bool end() override;
        bool reset(bool release_resources = false) override;

        void draw(
            uint32_t vertex_count,
            uint32_t instance_count = 1,
            uint32_t first_vertex   = 0,
            uint32_t first_instance = 0) override;

        void draw_indexed(
            uint32_t index_count,
            uint32_t instance_count = 1,
            uint32_t first_index    = 0,
            int32_t  vertex_offset  = 0,
            uint32_t first_instance = 0) override;

        void dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z) override;

        void bind_descriptor_set(rhi::DescriptorSet* set, uint32_t set_index) override;
        void bind_descriptor_sets(const std::vector<rhi::DescriptorSet*>& sets, uint32_t first_set) override;

        void pipeline_image_barrier(
            VkImage       image,
            VkImageLayout old_layout,
            VkImageLayout new_layout,

            uint32_t src_stage_mask,
            uint32_t src_access_mask,
            uint32_t dst_stage_mask,
            uint32_t dst_access_mask,

            VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
            uint32_t base_mip_level   = 0,
            uint32_t level_count      = 1,
            uint32_t base_array_layer = 0,
            uint32_t layer_count      = 1) const;

        [[nodiscard]] VkCommandBuffer vk_command_buffer() const;

    private:
        explicit CommandBuffer(const CommandBufferDesc& desc) : rhi::CommandBuffer(desc) {}

        void set_vk_command_buffer(VkCommandBuffer cmd_buffer);
        VkCommandBuffer vk_command_buffer_{ nullptr };

        friend class CommandPool;
    };

    class CommandQueue final : public rhi::CommandQueue
    {
    public:
        bool init() override;
        void destroy() override;

        bool submit(const SubmitInfo& submit_info) override;
        bool submit(
            const std::vector<rhi::CommandBuffer*>& command_buffers,
            rhi::Fence*                                  signal_fence = nullptr) override;

        PresentResult present(const PresentInfo& present_info) override;
        PresentResult present(
            rhi::Swapchain* swapchain,
            uint32_t image_index,
            const std::vector<rhi::Semaphore*>& wait_semaphores) override;

        bool wait_idle() override;

        [[nodiscard]] VkQueue vk_queue() const;

    private:
        explicit CommandQueue(const CommandQueueDesc& desc) : rhi::CommandQueue(desc) {}

        VkQueue vk_queue_{ nullptr };

        friend GraphicsObject;
    };
}
