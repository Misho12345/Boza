module boza.rhi.vulkan;

import :command;
import :util;
import :pipeline;
import :resources;
import :shader_module;

namespace boza::rhi::vk
{
    bool CommandBuffer::init() { return true; }
    void CommandBuffer::destroy() { vk_command_buffer_ = nullptr; }

    bool CommandBuffer::begin() { return begin(desc.usage); }

    bool CommandBuffer::begin(const Flags<CommandBufferUsage> usage_flags)
    {
        // Log::trace("Beginning command buffer recording");

        VkCommandBufferUsageFlags vk_flags{};

        if (usage_flags.has(CommandBufferUsage::OneTimeSubmit)) vk_flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (usage_flags.has(CommandBufferUsage::RenderPassContinue)) vk_flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        if (usage_flags.has(CommandBufferUsage::SimultaneousUse)) vk_flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        const VkCommandBufferBeginInfo begin_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = vk_flags,
            .pInheritanceInfo = nullptr
        };

        if (!vk_check(
            vkBeginCommandBuffer(vk_command_buffer_, &begin_info),
            "Failed to begin command buffer"))
            return false;

        return true;
    }

    bool CommandBuffer::end()
    {
        // Log::trace("Ending command buffer recording");

        if (!vk_check(
            vkEndCommandBuffer(vk_command_buffer_),
            "Failed to end command buffer"))
            return false;

        return true;
    }


    bool CommandBuffer::reset(const bool release_resources)
    {
        // Log::trace("Resetting command buffer (release_resources: {})", release_resources);

        const VkCommandBufferResetFlags flags = release_resources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0;

        if (!vk_check(
            vkResetCommandBuffer(vk_command_buffer_, flags),
            "Failed to reset command buffer"))
            return false;

        return true;
    }


    void CommandBuffer::draw(
        const uint32_t vertex_count,
        const uint32_t instance_count,
        const uint32_t first_vertex,
        const uint32_t first_instance)
    {
        // Log::trace("Draw call: {} vertices, {} instances", vertex_count, instance_count);
        vkCmdDraw(vk_command_buffer_, vertex_count, instance_count, first_vertex, first_instance);
    }

    void CommandBuffer::draw_indexed(
        const uint32_t index_count,
        const uint32_t instance_count,
        const uint32_t first_index,
        const int32_t  vertex_offset,
        const uint32_t first_instance)
    {
        // Log::trace("Draw indexed call: {} indices, {} instances", index_count, instance_count);
        vkCmdDrawIndexed(vk_command_buffer_, index_count, instance_count, first_index, vertex_offset, first_instance);
    }


    void CommandBuffer::dispatch(const uint32_t group_x, const uint32_t group_y, const uint32_t group_z)
    {
        // Log::trace("Dispatch compute: {}x{}x{} groups", group_x, group_y, group_z);
        vkCmdDispatch(vk_command_buffer_, group_x, group_y, group_z);
    }

    void CommandBuffer::bind_graphics_pipeline(rhi::GraphicsPipeline* pipeline)
    {
        // Log::trace("Binding graphics pipeline");
        const auto* vk_pipeline = reinterpret_cast<GraphicsPipeline*>(pipeline);
        vkCmdBindPipeline(vk_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->vk_pipeline());
        current_pipeline_bind_point_ = VK_PIPELINE_BIND_POINT_GRAPHICS;
    }

    void CommandBuffer::bind_compute_pipeline(rhi::ComputePipeline* pipeline)
    {
        // Log::trace("Binding compute pipeline");
        const auto* vk_pipeline = reinterpret_cast<ComputePipeline*>(pipeline);
        vkCmdBindPipeline(vk_command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, vk_pipeline->vk_pipeline());
        current_pipeline_bind_point_ = VK_PIPELINE_BIND_POINT_COMPUTE;
    }

    void CommandBuffer::bind_descriptor_set(
        rhi::PipelineLayout* layout,
        rhi::DescriptorSet*  set,
        const uint32_t       set_index)
    {
        // Log::trace("Binding descriptor set at index {}", set_index);
        bind_descriptor_sets(layout, { set }, set_index);
    }

    void CommandBuffer::bind_descriptor_sets(
        rhi::PipelineLayout*                    layout,
        const std::vector<rhi::DescriptorSet*>& sets,
        const uint32_t                          first_set)
    {
        // Log::trace("Binding {} descriptor set(s) starting at index {}", sets.size(), first_set);

        if (!layout)
        {
            Log::warn("Cannot bind descriptor sets without a pipeline layout");
            return;
        }

        const auto* vk_layout = reinterpret_cast<vk::PipelineLayout*>(layout);

        std::vector<VkDescriptorSet> vk_sets;
        vk_sets.reserve(sets.size());

        for (const auto& set : sets) { vk_sets.push_back(reinterpret_cast<DescriptorSet*>(set)->vk_descriptor_set()); }

        vkCmdBindDescriptorSets(
            vk_command_buffer_,
            current_pipeline_bind_point_,
            vk_layout->vk_pipeline_layout(),
            first_set,
            static_cast<uint32_t>(vk_sets.size()),
            vk_sets.data(),
            0,
            nullptr);
    }

    void CommandBuffer::bind_vertex_buffer(rhi::Buffer* buffer, const uint32_t binding, const uint64_t offset)
    {
        // Log::trace("Binding vertex buffer at binding {} with offset {}", binding, offset);
        const auto*        vk_buffer = reinterpret_cast<Buffer*>(buffer);
        const VkBuffer     vk_buf    = vk_buffer->vk_buffer();
        const VkDeviceSize vk_offset = offset;
        vkCmdBindVertexBuffers(vk_command_buffer_, binding, 1, &vk_buf, &vk_offset);
    }

    void CommandBuffer::bind_index_buffer(rhi::Buffer* buffer, const uint64_t offset, const bool use_uint16)
    {
        // Log::trace("Binding index buffer with offset {} ({})", offset, use_uint16 ? "uint16" : "uint32");
        const auto*       vk_buffer  = reinterpret_cast<Buffer*>(buffer);
        const VkIndexType index_type = use_uint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
        vkCmdBindIndexBuffer(vk_command_buffer_, vk_buffer->vk_buffer(), offset, index_type);
    }

    void CommandBuffer::push_constants(
        rhi::PipelineLayout*   layout,
        const rhi::ShaderStage stage,
        const uint32_t         offset,
        const uint32_t         size,
        const void*            data)
    {
        // Log::trace("Pushing constants: {} bytes at offset {}", size, offset);
        const auto* vk_layout = reinterpret_cast<vk::PipelineLayout*>(layout);

        VkShaderStageFlags stage_flags = 0;
        if (static_cast<uint8_t>(stage) & static_cast<uint8_t>(rhi::ShaderStage::Vertex)) stage_flags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (static_cast<uint8_t>(stage) & static_cast<uint8_t>(rhi::ShaderStage::Fragment)) stage_flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (static_cast<uint8_t>(stage) & static_cast<uint8_t>(rhi::ShaderStage::Compute)) stage_flags |= VK_SHADER_STAGE_COMPUTE_BIT;

        vkCmdPushConstants(vk_command_buffer_, vk_layout->vk_pipeline_layout(), stage_flags, offset, size, data);
    }


    void CommandBuffer::pipeline_image_barrier(
        const VkImage       image,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,

        const uint32_t src_stage_mask,
        const uint32_t src_access_mask,
        const uint32_t dst_stage_mask,
        const uint32_t dst_access_mask,

        const VkImageAspectFlags aspect_mask,
        const uint32_t           base_mip_level,
        const uint32_t           level_count,
        const uint32_t           base_array_layer,
        const uint32_t           layer_count) const
    {
        // Log::trace("Pipeline image barrier: layout transition {} -> {}", static_cast<uint32_t>(old_layout), static_cast<uint32_t>(new_layout));

        VkImageMemoryBarrier2 barrier
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = static_cast<VkPipelineStageFlags2>(src_stage_mask),
            .srcAccessMask = static_cast<VkAccessFlags2>(src_access_mask),
            .dstStageMask = static_cast<VkPipelineStageFlags2>(dst_stage_mask),
            .dstAccessMask = static_cast<VkAccessFlags2>(dst_access_mask),
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = vk_queue_family_ignored,
            .dstQueueFamilyIndex = vk_queue_family_ignored,
            .image = image,
            .subresourceRange = {
                .aspectMask = aspect_mask,
                .baseMipLevel = base_mip_level,
                .levelCount = level_count,
                .baseArrayLayer = base_array_layer,
                .layerCount = layer_count
            }
        };

        const VkDependencyInfo dependency_info
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
        };

        vkCmdPipelineBarrier2(vk_command_buffer_, &dependency_info);
    }


    VkCommandBuffer CommandBuffer::vk_command_buffer() const { return vk_command_buffer_; }

    void CommandBuffer::set_vk_command_buffer(const VkCommandBuffer cmd_buffer) { vk_command_buffer_ = cmd_buffer; }
}
