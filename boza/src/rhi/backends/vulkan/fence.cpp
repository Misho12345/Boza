module boza.rhi.vulkan;

import :sync;
import :util;

namespace boza::rhi::vk
{
    bool Fence::init()
    {
        // Log::trace("Creating vulkan fence");

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        const VkFenceCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = (desc_.signaled ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlags{})
        };

        if (!vk_check(
            vkCreateFence(vk_device, &create_info, nullptr, &vk_fence_),
            "Failed to create fence"))
            return false;

        return true;
    }

    void Fence::destroy()
    {
        // Log::trace("Destroying vulkan fence");
        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        if (vk_fence_)
        {
            vkDestroyFence(vk_device, vk_fence_, nullptr);
            vk_fence_ = nullptr;
        }
    }

    bool Fence::wait(const std::uint64_t timeout)
    {
        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        const VkResult result = vkWaitForFences(vk_device, 1, &vk_fence_, true, timeout);

        if (result == VK_SUCCESS) return true;
        if (result == VK_TIMEOUT) return false;

        if (!vk_check(result, "Failed to wait for fence")) return false;

        return false;
    }

    bool Fence::reset()
    {
        // Log::trace("Resetting fence");

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        if (!vk_check(
            vkResetFences(vk_device, 1, &vk_fence_),
            "Failed to reset fence"))
            return false;

        return true;
    }

    bool Fence::is_signaled() const
    {
        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        return vkGetFenceStatus(vk_device, vk_fence_) == VK_SUCCESS;
    }

    VkFence Fence::vk_fence() const { return vk_fence_; }
}
