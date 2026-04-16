module boza.rhi.vulkan;

import :command;
import :util;
import :sync;

namespace boza::rhi::vk
{
    bool CommandQueue::init()
    {
        // Log::trace("Initializing command queue ({})", desc.family_index);

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        vkGetDeviceQueue(vk_device, desc_.family_index, 0, &vk_queue_);
        return vk_queue_;
    }

    void CommandQueue::destroy() { vk_queue_ = nullptr; }

    bool CommandQueue::submit(const SubmitInfo& submit_info)
    {
        // Log::trace("Submitting {} command buffer(s) to queue", submit_info.command_buffers.size());

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

        std::vector<VkPipelineStageFlags2> vk_wait_stages;
        vk_wait_stages.reserve(vk_wait_semaphores.size());

        if (!submit_info.wait_stages.empty() && submit_info.wait_stages.size() != vk_wait_semaphores.size())
        {
            Log::critical("Wait semaphore count must match wait stage count");
            return false;
        }

        if (submit_info.wait_stages.empty())
        {
            vk_wait_stages.resize(vk_wait_semaphores.size(), to_vk(Flags<PipelineStage>{ PipelineStage::AllCommands }));
        }
        else
        {
            for (const auto stage_mask : submit_info.wait_stages)
            {
                VkPipelineStageFlags2 vk_stage_mask = to_vk(stage_mask);
                if (vk_stage_mask == VK_PIPELINE_STAGE_2_NONE) vk_stage_mask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
                vk_wait_stages.push_back(vk_stage_mask);
            }
        }

        if (!submit_info.wait_values.empty() && submit_info.wait_values.size() != vk_wait_semaphores.size())
        {
            Log::critical("Wait semaphore count must match wait value count");
            return false;
        }

        if (!submit_info.signal_values.empty() && submit_info.signal_values.size() != vk_signal_semaphores.size())
        {
            Log::critical("Signal semaphore count must match signal value count");
            return false;
        }

        const VkFence vk_fence = submit_info.signal_fence ? reinterpret_cast<Fence*>(submit_info.signal_fence)->vk_fence() : nullptr;

        std::vector<VkSemaphoreSubmitInfo> vk_wait_infos;
        vk_wait_infos.reserve(vk_wait_semaphores.size());
        for (std::size_t i = 0; i < vk_wait_semaphores.size(); ++i)
        {
            const SemaphoreType sem_type = submit_info.wait_semaphores[i]->type();
            const bool is_timeline = sem_type == SemaphoreType::Timeline;
            const std::uint64_t wait_value = submit_info.wait_values.empty() ? 0 : submit_info.wait_values[i];

            if (is_timeline && submit_info.wait_values.empty())
            {
                Log::critical("Timeline wait semaphore at index {} requires a wait value", i);
                return false;
            }

            if (!is_timeline && !submit_info.wait_values.empty() && wait_value != 0)
            {
                Log::critical("Binary wait semaphore at index {} must use value 0", i);
                return false;
            }

            vk_wait_infos.push_back({
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = vk_wait_semaphores[i],
                .value = wait_value,
                .stageMask = vk_wait_stages[i],
                .deviceIndex = 0
            });
        }

        std::vector<VkCommandBufferSubmitInfo> vk_cmd_buffer_infos;
        vk_cmd_buffer_infos.reserve(vk_cmd_buffers.size());
        for (const VkCommandBuffer vk_cmd_buffer : vk_cmd_buffers)
        {
            vk_cmd_buffer_infos.push_back({
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext = nullptr,
                .commandBuffer = vk_cmd_buffer,
                .deviceMask = 0
            });
        }

        std::vector<VkSemaphoreSubmitInfo> vk_signal_infos;
        vk_signal_infos.reserve(vk_signal_semaphores.size());
        for (std::size_t i = 0; i < vk_signal_semaphores.size(); ++i)
        {
            const SemaphoreType sem_type = submit_info.signal_semaphores[i]->type();
            const bool is_timeline = sem_type == SemaphoreType::Timeline;
            const std::uint64_t signal_value = submit_info.signal_values.empty() ? 0 : submit_info.signal_values[i];

            if (is_timeline && submit_info.signal_values.empty())
            {
                Log::critical("Timeline signal semaphore at index {} requires a signal value", i);
                return false;
            }

            if (!is_timeline && !submit_info.signal_values.empty() && signal_value != 0)
            {
                Log::critical("Binary signal semaphore at index {} must use value 0", i);
                return false;
            }

            vk_signal_infos.push_back({
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = vk_signal_semaphores[i],
                .value = signal_value,
                .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .deviceIndex = 0
            });
        }

        const VkSubmitInfo2 vk_submit_info
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = static_cast<std::uint32_t>(vk_wait_infos.size()),
            .pWaitSemaphoreInfos = vk_wait_infos.empty() ? nullptr : vk_wait_infos.data(),
            .commandBufferInfoCount = static_cast<std::uint32_t>(vk_cmd_buffer_infos.size()),
            .pCommandBufferInfos = vk_cmd_buffer_infos.data(),
            .signalSemaphoreInfoCount = static_cast<std::uint32_t>(vk_signal_infos.size()),
            .pSignalSemaphoreInfos = vk_signal_infos.empty() ? nullptr : vk_signal_infos.data()
        };

        if (!vk_check(
            vkQueueSubmit2(vk_queue_, 1, &vk_submit_info, vk_fence),
            "Failed to submit command buffers to queue"))
            return false;

        return true;
    }

    PresentResult CommandQueue::present(const PresentInfo& present_info)
    {
        // Log::trace("Presenting {} swapchain(s)", present_info.swapchains.size());

        if (present_info.swapchains.empty()) return PresentResult::Success;

        if (present_info.swapchains.size() != present_info.image_indices.size())
        {
            Log::critical("Swapchain count must match image index count");
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
            .waitSemaphoreCount = static_cast<std::uint32_t>(vk_wait_semaphores.size()),
            .pWaitSemaphores = vk_wait_semaphores.empty() ? nullptr : vk_wait_semaphores.data(),
            .swapchainCount = static_cast<std::uint32_t>(vk_swapchains.size()),
            .pSwapchains = vk_swapchains.data(),
            .pImageIndices = present_info.image_indices.data(),
            .pResults = nullptr
        };

        const VkResult result = vkQueuePresentKHR(vk_queue_, &vk_present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) return PresentResult::OutOfDate;
        if (result == VK_SUBOPTIMAL_KHR) return PresentResult::Suboptimal;

        if (!vk_check(result, "Failed to present swapchain image")) return PresentResult::Error;

        return PresentResult::Success;
    }

    bool CommandQueue::wait_idle()
    {
        // Log::trace("Waiting for queue to become idle");

        return vk_check(
            vkQueueWaitIdle(vk_queue_),
            "Failed to wait for queue to become idle");
    }

    VkQueue CommandQueue::vk_queue() const { return vk_queue_; }
}
