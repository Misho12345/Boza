#include "Sync.hpp"
#include "Device.hpp"

namespace boza::rhi::vk
{
    /// ---------------------
    /// ===== Semaphore =====
    /// ---------------------

    bool Semaphore::init()
    {
        Logger::trace("Creating vulkan semaphore");

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        if (desc.type == SemaphoreType::Timeline)
        {
            VkSemaphoreTypeCreateInfo type_info
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                .pNext = nullptr,
                .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
                .initialValue = desc.value
            };

            const VkSemaphoreCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = &type_info,
                .flags = {}
            };

            VK_CHECK(vkCreateSemaphore(vk_device, &create_info, nullptr, &vk_semaphore_),
            {
                LOG_VK_ERROR("Failed to create timeline semaphore");
                return false;
            });
        }
        else
        {
            constexpr VkSemaphoreCreateInfo create_info
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {}
            };

            VK_CHECK(vkCreateSemaphore(vk_device, &create_info, nullptr, &vk_semaphore_),
            {
                LOG_VK_ERROR("Failed to create binary semaphore");
                return false;
            });
        }

        return true;
    }

    void Semaphore::destroy()
    {
        Logger::trace("Destroying vulkan semaphore");

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (vk_semaphore_)
        {
            vkDestroySemaphore(vk_device, vk_semaphore_, nullptr);
            vk_semaphore_ = nullptr;
        }
    }


    bool Semaphore::signal(const uint64_t value)
    {
        if (desc.type != SemaphoreType::Timeline)
        {
            Logger::critical("Cannot signal a binary semaphore from host");
            return false;
        }

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        const VkSemaphoreSignalInfo signal_info
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
            .pNext = nullptr,
            .semaphore = vk_semaphore_,
            .value = value
        };

        VK_CHECK(vkSignalSemaphore(vk_device, &signal_info),
        {
            LOG_VK_ERROR("Failed to signal timeline semaphore");
            return false;
        });

        return true;
    }

    bool Semaphore::wait(uint64_t value, const uint64_t timeout)
    {
        if (desc.type != SemaphoreType::Timeline)
        {
            Logger::critical("Cannot wait on a binary semaphore from host");
            return false;
        }

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        const VkSemaphoreWaitInfo wait_info
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .pNext = nullptr,
            .flags = 0,
            .semaphoreCount = 1,
            .pSemaphores = &vk_semaphore_,
            .pValues = &value
        };

        VK_CHECK(vkWaitSemaphores(vk_device, &wait_info, timeout),
        {
            LOG_VK_ERROR("Failed to wait on timeline semaphore");
            return false;
        });

        return true;
    }

    uint64_t Semaphore::counter_value() const
    {
        if (desc.type != SemaphoreType::Timeline)
        {
            Logger::critical("Cannot get counter value from binary semaphore");
            return 0;
        }

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        uint64_t value = 0;

        VK_CHECK(vkGetSemaphoreCounterValue(vk_device, vk_semaphore_, &value),
        {
            LOG_VK_ERROR("Failed to get semaphore counter value");
            return 0;
        });

        return value;
    }

    VkSemaphore Semaphore::vk_semaphore() const { return vk_semaphore_; }

    /// -----------------
    /// ===== Fence =====
    /// -----------------

    bool Fence::init()
    {
        Logger::trace("Creating vulkan fence");

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        const VkFenceCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = (desc.signaled ? VK_FENCE_CREATE_SIGNALED_BIT : VkFenceCreateFlags{})
        };

        VK_CHECK(vkCreateFence(vk_device, &create_info, nullptr, &vk_fence_),
        {
            LOG_VK_ERROR("Failed to create fence");
            return false;
        });

        return true;
    }

    void Fence::destroy()
    {
        Logger::trace("Destroying vulkan fence");
        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (vk_fence_)
        {
            vkDestroyFence(vk_device, vk_fence_, nullptr);
            vk_fence_ = nullptr;
        }
    }

    bool Fence::wait(const uint64_t timeout)
    {
        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        VK_CHECK(vkWaitForFences(vk_device, 1, &vk_fence_, VK_TRUE, timeout),
        {
            LOG_VK_ERROR("Failed to wait for fence");
            return false;
        });

        return true;
    }

    bool Fence::reset()
    {
        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();

        VK_CHECK(vkResetFences(vk_device, 1, &vk_fence_),
        {
            LOG_VK_ERROR("Failed to reset fence");
            return false;
        });

        return true;
    }

    bool Fence::is_signaled() const
    {
        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        return vkGetFenceStatus(vk_device, vk_fence_) == VK_SUCCESS;
    }

    VkFence Fence::vk_fence() const { return vk_fence_; }
}
