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

    bool CommandBuffer::begin() { return begin(desc_.usage); }

    bool CommandBuffer::begin(const Flags<CommandBufferUsage> usage_flags)
    {
        // Log::trace("Beginning command buffer recording");

        VkCommandBufferUsageFlags vk_flags{};

        if (usage_flags & CommandBufferUsage::OneTimeSubmit) vk_flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (usage_flags & CommandBufferUsage::RenderPassContinue) vk_flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        if (usage_flags & CommandBufferUsage::SimultaneousUse) vk_flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

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
        const std::uint32_t vertex_count,
        const std::uint32_t instance_count,
        const std::uint32_t first_vertex,
        const std::uint32_t first_instance)
    {
        // Log::trace("Draw call: {} vertices, {} instances", vertex_count, instance_count);
        vkCmdDraw(vk_command_buffer_, vertex_count, instance_count, first_vertex, first_instance);
    }

    void CommandBuffer::draw_indexed(
        const std::uint32_t index_count,
        const std::uint32_t instance_count,
        const std::uint32_t first_index,
        const std::int32_t  vertex_offset,
        const std::uint32_t first_instance)
    {
        // Log::trace("Draw indexed call: {} indices, {} instances", index_count, instance_count);
        vkCmdDrawIndexed(vk_command_buffer_, index_count, instance_count, first_index, vertex_offset, first_instance);
    }

    void CommandBuffer::draw_indexed_indirect(
        rhi::Buffer*        buffer,
        const std::uint64_t offset,
        const std::uint32_t draw_count,
        const std::uint32_t stride)
    {
        const auto* vk_buffer = reinterpret_cast<Buffer*>(buffer);
        if (!vk_buffer)
        {
            Log::warn("Cannot draw indexed indirect with a null buffer");
            return;
        }

        vkCmdDrawIndexedIndirect(
            vk_command_buffer_,
            vk_buffer->vk_buffer(),
            offset,
            draw_count,
            stride);
    }

    void CommandBuffer::dispatch(const std::uint32_t group_x, const std::uint32_t group_y, const std::uint32_t group_z)
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

    void CommandBuffer::bind_descriptor_sets(
        rhi::PipelineLayout*                 layout,
        const std::span<rhi::DescriptorSet*> sets,
        const std::uint32_t                  first_set)
    {
        // Log::trace("Binding {} descriptor set(s) starting at index {}", sets.size(), first_set);

        if (!layout)
        {
            Log::warn("Cannot bind descriptor sets without a pipeline layout");
            return;
        }

        const auto* vk_layout = reinterpret_cast<PipelineLayout*>(layout);

        std::vector<VkDescriptorSet> vk_sets;
        vk_sets.reserve(sets.size());

        for (const auto& set : sets) { vk_sets.push_back(reinterpret_cast<DescriptorSet*>(set)->vk_descriptor_set()); }

        vkCmdBindDescriptorSets(
            vk_command_buffer_,
            current_pipeline_bind_point_,
            vk_layout->vk_pipeline_layout(),
            first_set,
            static_cast<std::uint32_t>(vk_sets.size()),
            vk_sets.data(),
            0,
            nullptr);
    }

    void CommandBuffer::bind_vertex_buffer(rhi::Buffer* buffer, const std::uint32_t binding, const std::uint64_t offset)
    {
        // Log::trace("Binding vertex buffer at binding {} with offset {}", binding, offset);
        const auto*        vk_buffer = reinterpret_cast<Buffer*>(buffer);
        const VkBuffer     vk_buf    = vk_buffer->vk_buffer();
        const VkDeviceSize vk_offset = offset;
        vkCmdBindVertexBuffers(vk_command_buffer_, binding, 1, &vk_buf, &vk_offset);
    }

    void CommandBuffer::bind_index_buffer(
        rhi::Buffer*     buffer,
        const std::uint64_t offset,
        const IndexType  index_type)
    {
        // Log::trace("Binding index buffer with offset {}", offset);
        const auto*       vk_buffer  = reinterpret_cast<Buffer*>(buffer);

        VkIndexType vk_index_type = VK_INDEX_TYPE_UINT32;
        if (index_type == IndexType::Uint16) vk_index_type = VK_INDEX_TYPE_UINT16;

        vkCmdBindIndexBuffer(vk_command_buffer_, vk_buffer->vk_buffer(), offset, vk_index_type);
    }

    void CommandBuffer::push_constants(
        rhi::PipelineLayout* layout,
        const ShaderStage    stage,
        const std::uint32_t  offset,
        const std::uint32_t  size,
        const void*          data)
    {
        // Log::trace("Pushing constants: {} bytes at offset {}", size, offset);
        const auto* vk_layout = reinterpret_cast<PipelineLayout*>(layout);

        VkShaderStageFlags stage_flags = 0;
        if (static_cast<std::uint8_t>(stage) & static_cast<std::uint8_t>(ShaderStage::Vertex)) stage_flags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (static_cast<std::uint8_t>(stage) & static_cast<std::uint8_t>(ShaderStage::Fragment)) stage_flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (static_cast<std::uint8_t>(stage) & static_cast<std::uint8_t>(ShaderStage::Compute)) stage_flags |= VK_SHADER_STAGE_COMPUTE_BIT;
        if (static_cast<std::uint8_t>(stage) & static_cast<std::uint8_t>(ShaderStage::TessControl)) stage_flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (static_cast<std::uint8_t>(stage) & static_cast<std::uint8_t>(ShaderStage::TessEvaluation)) stage_flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (static_cast<std::uint8_t>(stage) & static_cast<std::uint8_t>(ShaderStage::Geometry)) stage_flags |= VK_SHADER_STAGE_GEOMETRY_BIT;

        vkCmdPushConstants(vk_command_buffer_, vk_layout->vk_pipeline_layout(), stage_flags, offset, size, data);
    }

    void CommandBuffer::begin_depth_rendering(
        rhi::Texture*       depth_texture,
        const std::uint32_t width,
        const std::uint32_t height,
        const float         clear_depth,
        const bool          flip_y)
    {
        begin_depth_rendering_layer(depth_texture, 0, width, height, clear_depth, flip_y);
    }

    void CommandBuffer::begin_depth_rendering_layer(
        rhi::Texture*       depth_texture,
        const std::uint32_t layer,
        const std::uint32_t width,
        const std::uint32_t height,
        const float         clear_depth,
        const bool          flip_y)
    {
        const auto* vk_texture = reinterpret_cast<Texture*>(depth_texture);
        if (!vk_texture)
        {
            Log::warn("Cannot begin depth rendering with a null texture");
            return;
        }

        const VkClearValue clear_value{ .depthStencil = { clear_depth, 0 } };

        const VkRenderingAttachmentInfo depth_attachment
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = vk_texture->vk_layer_image_view(layer),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = nullptr,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = clear_value
        };

        const VkRenderingInfo rendering_info
        {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = {},
            .renderArea = { .offset = { 0, 0 }, .extent = { width, height } },
            .layerCount = 1,
            .viewMask = 0,
            .colorAttachmentCount = 0,
            .pColorAttachments = nullptr,
            .pDepthAttachment = &depth_attachment,
            .pStencilAttachment = nullptr
        };

        vkCmdBeginRendering(vk_command_buffer_, &rendering_info);

        const VkViewport viewport
        {
            .x = 0.0f,
            .y = flip_y ? static_cast<float>(height) : 0.0f,
            .width = static_cast<float>(width),
            .height = flip_y ? -static_cast<float>(height) : static_cast<float>(height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        const VkRect2D scissor
        {
            .offset = { 0, 0 },
            .extent = { width, height }
        };

        vkCmdSetViewport(vk_command_buffer_, 0, 1, &viewport);
        vkCmdSetScissor(vk_command_buffer_, 0, 1, &scissor);
    }

    void CommandBuffer::end_rendering()
    {
        vkCmdEndRendering(vk_command_buffer_);
    }

    void CommandBuffer::compute_memory_barrier()
    {
        static constexpr VkMemoryBarrier2 barrier
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT
        };

        constexpr VkDependencyInfo dependency_info
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &barrier,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 0,
            .pImageMemoryBarriers = nullptr
        };

        vkCmdPipelineBarrier2(vk_command_buffer_, &dependency_info);
    }

    void CommandBuffer::compute_to_draw_barrier()
    {
        static constexpr VkMemoryBarrier2 barrier
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
            .dstStageMask =
                VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT |
                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT
        };

        constexpr VkDependencyInfo dependency_info
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &barrier,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 0,
            .pImageMemoryBarriers = nullptr
        };

        vkCmdPipelineBarrier2(vk_command_buffer_, &dependency_info);
    }

    void CommandBuffer::draw_to_compute_barrier()
    {
        static constexpr VkMemoryBarrier2 barrier
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask =
                VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT |
                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .srcAccessMask = VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_2_SHADER_READ_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT
        };

        constexpr VkDependencyInfo dependency_info
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .dependencyFlags = 0,
            .memoryBarrierCount = 1,
            .pMemoryBarriers = &barrier,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 0,
            .pImageMemoryBarriers = nullptr
        };

        vkCmdPipelineBarrier2(vk_command_buffer_, &dependency_info);
    }

    void CommandBuffer::image_barrier(
        rhi::Texture*       texture,
        const ResourceState old_state,
        const ResourceState new_state)
    {
        const auto* vk_texture = reinterpret_cast<Texture*>(texture);
        if (!vk_texture)
        {
            Log::warn("Cannot insert image barrier for null texture");
            return;
        }

        auto [old_layout, src_stage] = state_to_layout_and_stage(old_state);
        auto [new_layout, dst_stage] = state_to_layout_and_stage(new_state);

        const VkAccessFlags2 src_access = state_to_access(old_state);
        const VkAccessFlags2 dst_access = state_to_access(new_state);

        pipeline_image_barrier(
            vk_texture->vk_image(),
            old_layout,
            new_layout,
            src_stage,
            src_access,
            dst_stage,
            dst_access,
            vk_texture->aspect_mask(),
            0,
            vk_texture->mip_levels(),
            0,
            vk_texture->layer_count());
    }

    void CommandBuffer::pipeline_image_barrier(
        const VkImage       image,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,

        const VkPipelineStageFlags2 src_stage_mask,
        const VkAccessFlags2        src_access_mask,
        const VkPipelineStageFlags2 dst_stage_mask,
        const VkAccessFlags2        dst_access_mask,

        const VkImageAspectFlags aspect_mask,
        const std::uint32_t    base_mip_level,
        const std::uint32_t    level_count,
        const std::uint32_t    base_array_layer,
        const std::uint32_t    layer_count) const
    {
        // Log::trace("Pipeline image barrier: layout transition {} -> {}", static_cast<std::uint32_t>(old_layout), static_cast<std::uint32_t>(new_layout));

        VkImageMemoryBarrier2 barrier
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = src_stage_mask,
            .srcAccessMask = src_access_mask,
            .dstStageMask = dst_stage_mask,
            .dstAccessMask = dst_access_mask,
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
