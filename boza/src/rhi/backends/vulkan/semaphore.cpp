module boza.rhi.vulkan;

import :sync;
import :util;

namespace boza::rhi::vk
{
    bool Semaphore::init()
    {
        // Log::trace("Creating vulkan semaphore");

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        if (desc_.type == SemaphoreType::Timeline)
        {
            VkSemaphoreTypeCreateInfo type_info
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                .pNext = nullptr,
                .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
                .initialValue = desc_.value
            };

            const VkSemaphoreCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = &type_info,
                .flags = {}
            };

            if (!vk_check(
                vkCreateSemaphore(vk_device, &create_info, nullptr, &vk_semaphore_),
                "Failed to create timeline semaphore"))
                return false;
        }
        else
        {
            static constexpr VkSemaphoreCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {}
            };

            if (!vk_check(
                vkCreateSemaphore(vk_device, &create_info, nullptr, &vk_semaphore_),
                "Failed to create binary semaphore"))
                return false;
        }

        return true;
    }

    void Semaphore::destroy()
    {
        // Log::trace("Destroying vulkan semaphore");

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        if (vk_semaphore_)
        {
            vkDestroySemaphore(vk_device, vk_semaphore_, nullptr);
            vk_semaphore_ = nullptr;
        }
    }


    bool Semaphore::signal(const uint64_t value)
    {
        if (desc_.type != SemaphoreType::Timeline)
        {
            Log::critical("Cannot signal a binary semaphore from host");
            return false;
        }

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        const VkSemaphoreSignalInfo signal_info
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
            .pNext = nullptr,
            .semaphore = vk_semaphore_,
            .value = value
        };

        if (!vk_check(
            vkSignalSemaphore(vk_device, &signal_info),
            "Failed to signal timeline semaphore"))
            return false;

        return true;
    }

    bool Semaphore::wait(uint64_t value, const uint64_t timeout)
    {
        if (desc_.type != SemaphoreType::Timeline)
        {
            Log::critical("Cannot wait on a binary semaphore from host");
            return false;
        }

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        const VkSemaphoreWaitInfo wait_info
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = {},
            .semaphoreCount = 1,
            .pSemaphores = &vk_semaphore_,
            .pValues = &value
        };

        if (!vk_check(
            vkWaitSemaphores(vk_device, &wait_info, timeout),
            "Failed to wait on timeline semaphore"))
            return false;

        return true;
    }

    uint64_t Semaphore::counter_value() const
    {
        if (desc_.type != SemaphoreType::Timeline)
        {
            Log::critical("Cannot get counter value from binary semaphore");
            return 0;
        }

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        uint64_t   value     = 0;

        if (!vk_check(
            vkGetSemaphoreCounterValue(vk_device, vk_semaphore_, &value),
            "Failed to get semaphore counter value"))
            return 0;

        return value;
    }

    VkSemaphore Semaphore::vk_semaphore() const { return vk_semaphore_; }
}
