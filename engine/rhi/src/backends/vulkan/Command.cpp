#include "Command.hpp"
#include "Pipeline.hpp"
#include "Descriptor.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"
#include "Sync.hpp"

namespace boza::rhi::vk
{
    /// ------------------------
    /// ===== Command Pool =====
    /// ------------------------

    bool CommandPool::init()
    {
        Logger::trace("Creating vulkan command pool ({})", desc.queue_family_index);

        const Device* device = reinterpret_cast<Device*>(desc.device);

        VkCommandPoolCreateFlags flags = 0;
        if (desc.flags.has(CommandPoolOption::Transient)) flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        if (desc.flags.has(CommandPoolOption::ResetCommandBuffer)) flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        const VkCommandPoolCreateInfo pool_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
            .queueFamilyIndex = desc.queue_family_index
        };

        VK_CHECK(vkCreateCommandPool(device->logical_device(), &pool_info, nullptr, &vk_command_pool_),
        {
            LOG_VK_ERROR("Failed to create command pool");
            return false;
        });

        return true;
    }

    void CommandPool::destroy()
    {
        Logger::trace("Destroying vulkan command pool ({})", desc.queue_family_index);

        const Device* device = reinterpret_cast<Device*>(desc.device);

        if (vk_command_pool_)
        {
            vkDestroyCommandPool(device->logical_device(), vk_command_pool_, nullptr);
            vk_command_pool_ = nullptr;
        }
    }


    rhi::CommandBuffer* CommandPool::allocate_command_buffer(const bool is_primary)
    {
        Logger::trace("Allocating command buffer for command pool ({})", desc.queue_family_index);

        const CommandBufferDesc cmd_desc
        {
            .device = desc.device,
            .pool = this,
            .is_primary = is_primary
        };

        auto* cmd_buffer = new CommandBuffer(cmd_desc);

        if (!cmd_buffer)
        {
            Logger::critical("Failed to allocate command buffer object");
            return nullptr;
        }

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        const VkCommandBufferAllocateInfo alloc_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = vk_command_pool_,
            .level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer vk_cmd_buffer;
        VK_CHECK(vkAllocateCommandBuffers(vk_device, &alloc_info, &vk_cmd_buffer),
        {
            LOG_VK_ERROR("Failed to allocate command buffer");
            delete cmd_buffer;
            return nullptr;
        });

        cmd_buffer->set_vk_command_buffer(vk_cmd_buffer);

        if (!cmd_buffer->init())
        {
            Logger::critical("Failed to initialize command buffer");
            vkFreeCommandBuffers(vk_device, vk_command_pool_, 1, &vk_cmd_buffer);
            delete cmd_buffer;
            return nullptr;
        }

        return cmd_buffer;
    }

    std::vector<rhi::CommandBuffer*> CommandPool::allocate_command_buffers(const uint32_t count, const bool is_primary)
    {
        Logger::trace("Allocating {} command buffers for command pool ({})", count, desc.queue_family_index);

        if (count == 0) return {};

        std::vector<rhi::CommandBuffer*> cmd_buffers;
        cmd_buffers.reserve(count);

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        const VkCommandBufferAllocateInfo alloc_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = vk_command_pool_,
            .level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            .commandBufferCount = count,
        };

        std::vector<VkCommandBuffer> vk_command_buffers(count);
        VK_CHECK(vkAllocateCommandBuffers(vk_device, &alloc_info, vk_command_buffers.data()),
        {
            LOG_VK_ERROR("Failed to allocate command buffers");
            return {};
        });

        for (uint32_t i = 0; i < count; ++i)
        {
            const CommandBufferDesc cmd_desc
            {
                .device = desc.device,
                .pool = this,
                .is_primary = is_primary
            };

            auto* cmd_buffer = new CommandBuffer(cmd_desc);
            if (!cmd_buffer)
            {
                Logger::critical("Failed to allocate command buffer object");
                for (auto* buf : cmd_buffers)
                {
                    buf->destroy();
                    delete buf;
                }
                vkFreeCommandBuffers(vk_device, vk_command_pool_, count, vk_command_buffers.data());
                return {};
            }

            cmd_buffer->set_vk_command_buffer(vk_command_buffers[i]);

            if (!cmd_buffer->init())
            {
                Logger::critical("Failed to initialize command buffer");
                delete cmd_buffer;
                for (auto* buf : cmd_buffers)
                {
                    buf->destroy();
                    delete buf;
                }
                vkFreeCommandBuffers(vk_device, vk_command_pool_, count, vk_command_buffers.data());
                return {};
            }

            cmd_buffers.push_back(cmd_buffer);
        }

        return cmd_buffers;
    }


    void CommandPool::free_command_buffer(rhi::CommandBuffer* command_buffer)
    {
        Logger::trace("Freeing command buffer for command pool ({})", desc.queue_family_index);
        if (!command_buffer) return;

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        const VkCommandBuffer vk_cmd = reinterpret_cast<CommandBuffer*>(command_buffer)->vk_command_buffer();

        vkFreeCommandBuffers(vk_device, vk_command_pool_, 1, &vk_cmd);
        command_buffer->destroy();
        delete command_buffer;
    }

    void CommandPool::free_command_buffers(const std::vector<rhi::CommandBuffer*>& command_buffers)
    {
        Logger::trace("Freeing {} command buffers for command pool ({})", command_buffers.size(), desc.queue_family_index);
        if (command_buffers.empty()) return;

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        std::vector<VkCommandBuffer> vk_cmd_buffers;
        vk_cmd_buffers.reserve(command_buffers.size());

        for (auto* cmd : command_buffers)
            if (cmd) vk_cmd_buffers.push_back(reinterpret_cast<CommandBuffer*>(cmd)->vk_command_buffer());

        vkFreeCommandBuffers(vk_device, vk_command_pool_,
                           static_cast<uint32_t>(vk_cmd_buffers.size()),
                           vk_cmd_buffers.data());

        for (auto* cmd : command_buffers)
        {
            if (cmd)
            {
                cmd->destroy();
                delete cmd;
            }
        }
    }


    bool CommandPool::reset(const bool release_resources)
    {
        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        const VkCommandPoolResetFlags flags = release_resources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0;

        VK_CHECK(vkResetCommandPool(vk_device, vk_command_pool_, flags),
        {
            LOG_VK_ERROR("Failed to reset command pool");
            return false;
        });

        return true;
    }

    rhi::CommandBuffer* CommandPool::begin_single_time_commands()
    {
        rhi::CommandBuffer* cmd_buffer = allocate_command_buffer(true);
        if (!cmd_buffer) return nullptr;

        if (!cmd_buffer->begin(CommandBufferUsage::OneTimeSubmit))
        {
            free_command_buffer(cmd_buffer);
            return nullptr;
        }

        return cmd_buffer;
    }

    bool CommandPool::end_single_time_commands(rhi::CommandBuffer* command_buffer)
    {
        if (!command_buffer) return false;

        const Device* device = reinterpret_cast<Device*>(desc.device);

        if (!command_buffer->end())
        {
            Logger::critical("Failed to end command buffer for single time commands");
            free_command_buffer(command_buffer);
            return false;
        }

        if (const auto queue = reinterpret_cast<CommandQueue*>(device->queue(desc.queue_family_index));
            !queue->submit({ command_buffer }) || !queue->wait_idle())
        {
            free_command_buffer(command_buffer);
            return false;
        }

        free_command_buffer(command_buffer);
        return true;
    }

    VkCommandPool CommandPool::vk_command_pool() const { return vk_command_pool_; }

    /// ---------------------------
    /// ===== Command Buffer ======
    // ----------------------------

    bool CommandBuffer::init() { return true; }
    void CommandBuffer::destroy() { vk_command_buffer_ = nullptr; }


    bool CommandBuffer::begin() { return begin(desc.usage); }

    bool CommandBuffer::begin(const Flags<CommandBufferUsage> usage_flags)
    {
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

        VK_CHECK(vkBeginCommandBuffer(vk_command_buffer_, &begin_info),
        {
            LOG_VK_ERROR("Failed to begin command buffer");
            return false;
        });

        return true;
    }

    bool CommandBuffer::end()
    {
        VK_CHECK(vkEndCommandBuffer(vk_command_buffer_),
        {
            LOG_VK_ERROR("Failed to end command buffer");
            return false;
        });

        return true;
    }


    bool CommandBuffer::reset(const bool release_resources)
    {
        const VkCommandBufferResetFlags flags = release_resources ? VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT : 0;

        VK_CHECK(vkResetCommandBuffer(vk_command_buffer_, flags),
        {
            LOG_VK_ERROR("Failed to reset command buffer");
            return false;
        });

        return true;
    }


    void CommandBuffer::draw(
        const uint32_t vertex_count,
        const uint32_t instance_count,
        const uint32_t first_vertex,
        const uint32_t first_instance)
    {
        vkCmdDraw(vk_command_buffer_, vertex_count, instance_count, first_vertex, first_instance);
    }

    void CommandBuffer::draw_indexed(
        const uint32_t index_count,
        const uint32_t instance_count,
        const uint32_t first_index,
        const int32_t  vertex_offset,
        const uint32_t first_instance)
    {
        vkCmdDrawIndexed(vk_command_buffer_, index_count, instance_count, first_index, vertex_offset, first_instance);
    }


    void CommandBuffer::dispatch(const uint32_t group_x, const uint32_t group_y, const uint32_t group_z)
    {
        vkCmdDispatch(vk_command_buffer_, group_x, group_y, group_z);
    }

    void CommandBuffer::bind_graphics_pipeline(rhi::GraphicsPipeline* pipeline)
    {
        const auto* vk_pipeline = reinterpret_cast<vk::GraphicsPipeline*>(pipeline);
        vkCmdBindPipeline(vk_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->vk_pipeline());
    }

    void CommandBuffer::bind_compute_pipeline(rhi::ComputePipeline* pipeline)
    {
        const auto* vk_pipeline = reinterpret_cast<vk::ComputePipeline*>(pipeline);
        vkCmdBindPipeline(vk_command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, vk_pipeline->vk_pipeline());
    }

    void CommandBuffer::bind_descriptor_set(rhi::PipelineLayout* layout, rhi::DescriptorSet* set, const uint32_t set_index)
    {
        bind_descriptor_sets(layout, { set }, set_index);
    }

    void CommandBuffer::bind_descriptor_sets(rhi::PipelineLayout* layout, const std::vector<rhi::DescriptorSet*>& sets, const uint32_t first_set)
    {
        if (!layout)
        {
            Logger::warn("Cannot bind descriptor sets without a pipeline layout");
            return;
        }

        const auto* vk_layout = reinterpret_cast<vk::PipelineLayout*>(layout);

        std::vector<VkDescriptorSet> vk_sets;
        vk_sets.reserve(sets.size());

        for (const auto& set : sets)
        {
            vk_sets.push_back(reinterpret_cast<DescriptorSet*>(set)->vk_descriptor_set());
        }

        vkCmdBindDescriptorSets(
            vk_command_buffer_,
            VK_PIPELINE_BIND_POINT_GRAPHICS, // TODO: Track bound pipeline type
            vk_layout->vk_pipeline_layout(),
            first_set,
            static_cast<uint32_t>(vk_sets.size()),
            vk_sets.data(),
            0,
            nullptr);
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
        const uint32_t base_mip_level,
        const uint32_t level_count,
        const uint32_t base_array_layer,
        const uint32_t layer_count) const
    {
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
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
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

    /// -------------------------
    /// ===== Command Queue =====
    /// -------------------------

    bool CommandQueue::init()
    {
        Logger::trace("Initializing command queue ({})", desc.family_index);

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        vkGetDeviceQueue(vk_device, desc.family_index, 0, &vk_queue_);
        return vk_queue_ != nullptr;
    }

    void CommandQueue::destroy() { vk_queue_ = nullptr; }

    bool CommandQueue::submit(const SubmitInfo& submit_info)
    {
        if (submit_info.command_buffers.empty()) return true;

        std::vector<VkCommandBuffer> vk_cmd_buffers;
        vk_cmd_buffers.reserve(submit_info.command_buffers.size());
        for (auto* cmd : submit_info.command_buffers)
        {
            vk_cmd_buffers.push_back(reinterpret_cast<CommandBuffer*>(cmd)->vk_command_buffer());
        }

        std::vector<VkSemaphore> vk_wait_semaphores;
        vk_wait_semaphores.reserve(submit_info.wait_semaphores.size());
        for (auto* sem : submit_info.wait_semaphores)
        {
            vk_wait_semaphores.push_back(reinterpret_cast<Semaphore*>(sem)->vk_semaphore());
        }

        std::vector<VkSemaphore> vk_signal_semaphores;
        vk_signal_semaphores.reserve(submit_info.signal_semaphores.size());
        for (auto* sem : submit_info.signal_semaphores)
        {
            vk_signal_semaphores.push_back(reinterpret_cast<Semaphore*>(sem)->vk_semaphore());
        }

        std::vector<VkPipelineStageFlags> vk_wait_stages;
        vk_wait_stages.reserve(submit_info.wait_stages.size());
        for (const uint32_t stage : submit_info.wait_stages)
        {
            vk_wait_stages.push_back(static_cast<VkPipelineStageFlags>(stage));
        }

        const VkFence vk_fence = submit_info.signal_fence ? reinterpret_cast<Fence*>(submit_info.signal_fence)->vk_fence() : nullptr;

        const VkSubmitInfo vk_submit_info
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast<uint32_t>(vk_wait_semaphores.size()),
            .pWaitSemaphores = vk_wait_semaphores.empty() ? nullptr : vk_wait_semaphores.data(),
            .pWaitDstStageMask = vk_wait_stages.empty() ? nullptr : vk_wait_stages.data(),
            .commandBufferCount = static_cast<uint32_t>(vk_cmd_buffers.size()),
            .pCommandBuffers = vk_cmd_buffers.data(),
            .signalSemaphoreCount = static_cast<uint32_t>(vk_signal_semaphores.size()),
            .pSignalSemaphores = vk_signal_semaphores.empty() ? nullptr : vk_signal_semaphores.data()
        };

        VK_CHECK(vkQueueSubmit(vk_queue_, 1, &vk_submit_info, vk_fence),
        {
            LOG_VK_ERROR("Failed to submit command buffers to queue");
            return false;
        });

        return true;
    }

    bool CommandQueue::submit(
        const std::vector<rhi::CommandBuffer*>& command_buffers,
        rhi::Fence* signal_fence)
    {
        const SubmitInfo submit_info
        {
            .command_buffers = command_buffers,
            .wait_semaphores = {},
            .wait_stages = {},
            .signal_semaphores = {},
            .signal_fence = signal_fence
        };

        return submit(submit_info);
    }


    PresentResult CommandQueue::present(const PresentInfo& present_info)
    {
        if (present_info.swapchains.empty()) return PresentResult::Success;

        if (present_info.swapchains.size() != present_info.image_indices.size())
        {
            Logger::critical("Swapchain count must match image index count");
            return PresentResult::Error;
        }

        std::vector<VkSwapchainKHR> vk_swapchains;
        vk_swapchains.reserve(present_info.swapchains.size());
        for (auto* swapchain : present_info.swapchains)
        {
            vk_swapchains.push_back(reinterpret_cast<Swapchain*>(swapchain)->vk_swapchain());
        }

        std::vector<VkSemaphore> vk_wait_semaphores;
        vk_wait_semaphores.reserve(present_info.wait_semaphores.size());
        for (auto* sem : present_info.wait_semaphores)
        {
            vk_wait_semaphores.push_back(reinterpret_cast<Semaphore*>(sem)->vk_semaphore());
        }

        const VkPresentInfoKHR vk_present_info
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = static_cast<uint32_t>(vk_wait_semaphores.size()),
            .pWaitSemaphores = vk_wait_semaphores.empty() ? nullptr : vk_wait_semaphores.data(),
            .swapchainCount = static_cast<uint32_t>(vk_swapchains.size()),
            .pSwapchains = vk_swapchains.data(),
            .pImageIndices = present_info.image_indices.data(),
            .pResults = nullptr
        };

        const VkResult result = vkQueuePresentKHR(vk_queue_, &vk_present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) return PresentResult::OutOfDate;
        if (result == VK_SUBOPTIMAL_KHR) return PresentResult::Suboptimal;

        if (result != VK_SUCCESS)
        {
            LOG_VK_ERROR("Failed to present swapchain image");
            return PresentResult::Error;
        }

        return PresentResult::Success;
    }

    PresentResult CommandQueue::present(
        rhi::Swapchain* swapchain,
        const uint32_t image_index,
        const std::vector<rhi::Semaphore*>& wait_semaphores)
    {
        const PresentInfo present_info
        {
            .swapchains = { swapchain },
            .image_indices = { image_index },
            .wait_semaphores = wait_semaphores
        };

        return present(present_info);
    }


    bool CommandQueue::wait_idle()
    {
        VK_CHECK(vkQueueWaitIdle(vk_queue_),
        {
            LOG_VK_ERROR("Failed to wait for queue to become idle");
            return false;
        });

        return true;
    }

    VkQueue CommandQueue::vk_queue() const { return vk_queue_; }
}