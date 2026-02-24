module boza.rhi.vulkan;

import :device;
import :resources;
import :util;
import :command;
import boza.core;

import <vk_all>;

namespace boza::rhi::vk
{
    bool Buffer::init()
    {
        // Determine buffer usage flags
        VkBufferUsageFlags usage_flags = to_vk(desc_.usage);

        // For DeviceLocal buffers, add transfer destination flag for staging
        if (desc_.memory_type == BufferMemoryType::DeviceLocal)
        {
            usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        const VkBufferCreateInfo buffer_create_info
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = desc_.size,
            .usage = usage_flags,
        };

        VmaAllocationCreateInfo allocation_create_info{};

        switch (desc_.memory_type)
        {
            case BufferMemoryType::DeviceLocal:
                allocation_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
                allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
                break;

            case BufferMemoryType::HostVisible:
                allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
                break;

            case BufferMemoryType::HostCoherent:
                allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                               VMA_ALLOCATION_CREATE_MAPPED_BIT;
                allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
                allocation_create_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
        }

        const auto allocator = reinterpret_cast<Device*>(desc_.device)->allocator()->vma_allocator();

        if (!vk_check(
            vmaCreateBuffer(
                allocator, &buffer_create_info, &allocation_create_info,
                &buffer_, &allocation_, &allocation_info_),
            "Failed to create buffer"))
            return false;

        return true;
    }

    void Buffer::destroy()
    {
        if (buffer_)
        {
            const auto allocator = reinterpret_cast<Device*>(desc_.device)->allocator()->vma_allocator();
            vmaDestroyBuffer(allocator, buffer_, allocation_);
            buffer_ = nullptr;
            allocation_ = nullptr;
        }
    }

    void* Buffer::map()
    {
        void* mapped_data;
        const auto allocator = reinterpret_cast<Device*>(desc_.device)->allocator()->vma_allocator();

        if (!vk_check(
            vmaMapMemory(allocator, allocation_, &mapped_data),
            "Failed to map buffer memory"))
            return nullptr;

        return mapped_data;
    }

    void Buffer::unmap()
    {
        const auto allocator = reinterpret_cast<Device*>(desc_.device)->allocator()->vma_allocator();
        vmaUnmapMemory(allocator, allocation_);
    }

    size_t Buffer::size() const { return desc_.size; }

    void Buffer::upload(const void* data, const size_t size, const size_t offset)
    {
        assert(offset + size <= desc_.size, "Upload out of bounds");

        // For DeviceLocal buffers, use staging buffer with transfer queue
        if (desc_.memory_type == BufferMemoryType::DeviceLocal)
        {
            const auto* device = reinterpret_cast<Device*>(desc_.device);

            const BufferDesc staging_desc
            {
                .device = desc_.device,
                .size = size,
                .usage = BufferUsage::Staging,
                .memory_type = BufferMemoryType::HostCoherent
            };

            const std::unique_ptr<Buffer> staging_buffer{ static_cast<Buffer*>(create<Buffer>(staging_desc)) };

            if (!staging_buffer)
            {
                Log::error("Failed to create staging buffer");
                return;
            }

            if (auto* mapped_data = staging_buffer->map())
            {
                std::memcpy(mapped_data, data, size);
                staging_buffer->unmap();
            }
            else
            {
                Log::error("Failed to map staging buffer");
                return;
            }

            std::uint32_t transfer_family = device->queue_family_indices().transfer_family;
            auto* command_pool = static_cast<CommandPool*>(device->command_pool(transfer_family));
            if (!command_pool)
            {
                Log::error("Failed to get transfer command pool for family {}", transfer_family);
                return;
            }

            auto* cmd_buffer = command_pool->begin_single_time_commands();
            if (!cmd_buffer)
            {
                Log::error("Failed to begin single time commands");
                return;
            }

            const VkBufferCopy copy_region
            {
                .srcOffset = 0,
                .dstOffset = offset,
                .size = size
            };

            auto* vk_cmd = reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer();
            vkCmdCopyBuffer(vk_cmd, staging_buffer->vk_buffer(), buffer_, 1, &copy_region);

            if (!command_pool->end_single_time_commands(cmd_buffer))
            {
                Log::error("Failed to end single time commands");
            }
        }
        else
        {
            // For host-visible buffers, directly map and copy
            if (const auto mapped_data = map())
            {
                std::memcpy(static_cast<char*>(mapped_data) + offset, data, size);
                unmap();
            }
        }
    }

    void Buffer::read_back(void* data, const size_t size, const size_t offset)
    {
        assert(
            desc_.memory_type != BufferMemoryType::DeviceLocal,
            "Cannot read back from device local buffer directly"
        );
        assert(offset + size <= desc_.size, "Read back out of bounds");

        if (const auto mapped_data = map())
        {
            std::memcpy(data, static_cast<const char*>(mapped_data) + offset, size);
            unmap();
        }
    }

    VkBuffer Buffer::vk_buffer() const { return buffer_; }
}
