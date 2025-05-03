#include "CommandPool.hpp"

#include "Device.hpp"
#include "Logger.hpp"

namespace boza
{
    bool CommandPool::create()
    {
        auto& inst = instance();

        const VkCommandPoolCreateInfo pool_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = Device::get_queue_family_indices().graphics_family
        };

        VK_CHECK(vkCreateCommandPool(Device::get_device(), &pool_info, nullptr, &inst.command_pool),
        {
            LOG_VK_ERROR("Failed to create command pool");
            return false;
        });

        const auto command_buffers = allocate_command_buffers(1);
        if (command_buffers.empty())
        {
            Logger::critical("Failed to allocate main command buffer");
            return false;
        }

        inst.command_buffer = command_buffers[0];
        return true;
    }

    void CommandPool::destroy()
    {
        vkDestroyCommandPool(Device::get_device(), get_command_pool(), nullptr);
    }


    std::vector<VkCommandBuffer> CommandPool::allocate_command_buffers(const uint32_t count)
    {
        const VkCommandBufferAllocateInfo alloc_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = instance().command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = count,
        };

        std::vector<VkCommandBuffer> command_buffers(count);
        VK_CHECK(vkAllocateCommandBuffers(Device::get_device(), &alloc_info, command_buffers.data()),
        {
            LOG_VK_ERROR("Failed to allocate command buffers");
            return {};
        });

        return command_buffers;
    }


    VkCommandBuffer CommandPool::begin_single_time_commands()
    {
        const VkCommandBufferAllocateInfo alloc_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = instance().command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VkCommandBuffer command_buffer;
        VK_CHECK(vkAllocateCommandBuffers(Device::get_device(), &alloc_info, &command_buffer),
        {
            LOG_VK_ERROR("Failed to allocate command buffer for single time commands");
            return nullptr;
        });

        constexpr VkCommandBufferBeginInfo begin_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };

        VK_CHECK(vkBeginCommandBuffer(command_buffer, &begin_info),
        {
            LOG_VK_ERROR("Failed to begin command buffer for single time commands");
            vkFreeCommandBuffers(Device::get_device(), instance().command_pool, 1, &command_buffer);
            return nullptr;
        });

        return command_buffer;
    }

    bool CommandPool::end_single_time_commands(VkCommandBuffer command_buffer)
    {
        if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        {
            Logger::error("Failed to end command buffer for single time commands");
            vkFreeCommandBuffers(Device::get_device(), instance().command_pool, 1, &command_buffer);
            return false;
        }

        const VkSubmitInfo submit_info
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr
        };

        if (vkQueueSubmit(Device::get_graphics_queue(), 1, &submit_info, nullptr) != VK_SUCCESS)
        {
            Logger::error("Failed to submit command buffer for single time commands");
            vkFreeCommandBuffers(Device::get_device(), instance().command_pool, 1, &command_buffer);
            return false;
        }

        if (vkQueueWaitIdle(Device::get_graphics_queue()) != VK_SUCCESS)
        {
            Logger::error("Failed to wait for graphics queue to become idle");
            return false;
        }

        vkFreeCommandBuffers(Device::get_device(), instance().command_pool, 1, &command_buffer);
        return true;
    }


    VkCommandPool&   CommandPool::get_command_pool() { return instance().command_pool; }
    VkCommandBuffer& CommandPool::get_command_buffer() { return instance().command_buffer; }
}
