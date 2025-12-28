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

        if (!vk_check(
            vkQueueSubmit(vk_queue_, 1, &vk_submit_info, vk_fence),
            "Failed to submit command buffers to queue"))
            return false;

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

        if (!vk_check(result, "Failed to present swapchain image")) return PresentResult::Error;

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
        // Log::trace("Waiting for queue to become idle");

        return vk_check(
            vkQueueWaitIdle(vk_queue_),
            "Failed to wait for queue to become idle");
    }

    VkQueue CommandQueue::vk_queue() const { return vk_queue_; }
}