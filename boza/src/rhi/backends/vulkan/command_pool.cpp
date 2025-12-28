module boza.rhi.vulkan;

import :command;
import :util;

namespace boza::rhi::vk
{
    bool CommandPool::init()
    {
        // Log::trace("Creating vulkan command pool ({})", desc.queue_family_index);

        const Device* device = reinterpret_cast<Device*>(desc_.device);

        VkCommandPoolCreateFlags flags = 0;
        if (desc_.flags.has(CommandPoolOption::Transient)) flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        if (desc_.flags.has(CommandPoolOption::ResetCommandBuffer)) flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        const VkCommandPoolCreateInfo pool_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = flags,
            .queueFamilyIndex = desc_.queue_family_index
        };

        if (!vk_check(
            vkCreateCommandPool(device->logical_device(), &pool_info, nullptr, &vk_command_pool_),
            "Failed to create command pool"))
            return false;

        return true;
    }

    void CommandPool::destroy()
    {
        // Log::trace("Destroying vulkan command pool ({})", desc.queue_family_index);

        const Device* device = reinterpret_cast<Device*>(desc_.device);

        if (vk_command_pool_)
        {
            vkDestroyCommandPool(device->logical_device(), vk_command_pool_, nullptr);
            vk_command_pool_ = nullptr;
        }
    }


    rhi::CommandBuffer* CommandPool::allocate_command_buffer(const bool is_primary)
    {
        // Log::trace("Allocating command buffer for command pool ({})", desc.queue_family_index);

        const CommandBufferDesc cmd_desc
        {
            .device = desc_.device,
            .pool = this,
            .is_primary = is_primary
        };

        auto* cmd_buffer = new CommandBuffer(cmd_desc);

        if (!cmd_buffer)
        {
            Log::critical("Failed to allocate command buffer object");
            return nullptr;
        }

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        const VkCommandBufferAllocateInfo alloc_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = vk_command_pool_,
            .level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer vk_cmd_buffer;
        if (!vk_check(
            vkAllocateCommandBuffers(vk_device, &alloc_info, &vk_cmd_buffer),
            "Failed to allocate command buffer"))
        {
            delete cmd_buffer;
            return nullptr;
        }

        cmd_buffer->set_vk_command_buffer(vk_cmd_buffer);

        if (!cmd_buffer->init())
        {
            Log::critical("Failed to initialize command buffer");
            vkFreeCommandBuffers(vk_device, vk_command_pool_, 1, &vk_cmd_buffer);
            delete cmd_buffer;
            return nullptr;
        }

        return cmd_buffer;
    }

    // TODO: remove duplication with allocate_command_buffer
    std::vector<rhi::CommandBuffer*> CommandPool::allocate_command_buffers(const uint32_t count, const bool is_primary)
    {
        // Log::trace("Allocating {} command buffers for command pool ({})", count, desc.queue_family_index);

        if (count == 0) return {};

        std::vector<rhi::CommandBuffer*> cmd_buffers;
        cmd_buffers.reserve(count);

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        const VkCommandBufferAllocateInfo alloc_info
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = vk_command_pool_,
            .level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
            .commandBufferCount = count,
        };

        std::vector<VkCommandBuffer> vk_command_buffers(count);
        if (!vk_check(
            vkAllocateCommandBuffers(vk_device, &alloc_info, vk_command_buffers.data()),
            "Failed to allocate command buffers"))
            return {};

        for (uint32_t i = 0; i < count; ++i)
        {
            const CommandBufferDesc cmd_desc
            {
                .device = desc_.device,
                .pool = this,
                .is_primary = is_primary
            };

            auto* cmd_buffer = new CommandBuffer(cmd_desc);
            if (!cmd_buffer)
            {
                Log::critical("Failed to allocate command buffer object");
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
                Log::critical("Failed to initialize command buffer");
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
        // Log::trace("Freeing command buffer for command pool ({})", desc.queue_family_index);

        if (!command_buffer) return;

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        const VkCommandBuffer vk_cmd = reinterpret_cast<CommandBuffer*>(command_buffer)->vk_command_buffer();

        vkFreeCommandBuffers(vk_device, vk_command_pool_, 1, &vk_cmd);
        command_buffer->destroy();
        delete command_buffer;
    }

    void CommandPool::free_command_buffers(const std::vector<rhi::CommandBuffer*>& command_buffers)
    {
        // Log::trace("Freeing {} command buffers for command pool ({})", command_buffers.size(), desc.queue_family_index);

        if (command_buffers.empty()) return;

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        std::vector<VkCommandBuffer> vk_cmd_buffers;
        vk_cmd_buffers.reserve(command_buffers.size());

        for (auto* cmd : command_buffers)
            if (cmd) vk_cmd_buffers.push_back(reinterpret_cast<CommandBuffer*>(cmd)->vk_command_buffer());

        vkFreeCommandBuffers(
            vk_device, vk_command_pool_,
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
        // Log::trace("Resetting command pool (release_resources: {})", release_resources);

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();

        const VkCommandPoolResetFlags flags = release_resources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0;

        if (!vk_check(
            vkResetCommandPool(vk_device, vk_command_pool_, flags),
            "Failed to reset command pool"))
            return false;

        return true;
    }

    rhi::CommandBuffer* CommandPool::begin_single_time_commands()
    {
        // Log::trace("Beginning single-time commands");

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
        // Log::trace("Ending single-time commands");

        if (!command_buffer) return false;

        const Device* device = reinterpret_cast<Device*>(desc_.device);

        if (!command_buffer->end())
        {
            Log::critical("Failed to end command buffer for single time commands");
            free_command_buffer(command_buffer);
            return false;
        }

        if (const auto queue = reinterpret_cast<CommandQueue*>(device->queue(desc_.queue_family_index));
            !queue->submit({ command_buffer }) || !queue->wait_idle())
        {
            free_command_buffer(command_buffer);
            return false;
        }

        free_command_buffer(command_buffer);
        return true;
    }

    VkCommandPool CommandPool::vk_command_pool() const { return vk_command_pool_; }
}