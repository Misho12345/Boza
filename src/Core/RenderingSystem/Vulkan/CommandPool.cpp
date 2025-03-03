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
\
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

    VkCommandPool&   CommandPool::get_command_pool() { return instance().command_pool; }
    VkCommandBuffer& CommandPool::get_command_buffer() { return instance().command_buffer; }
}
