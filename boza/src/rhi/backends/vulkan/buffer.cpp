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
        const auto* device = reinterpret_cast<Device*>(desc_.device);

        // Determine buffer usage flags
        VkBufferUsageFlags usage_flags = to_vk(desc_.usage);

        // For DeviceLocal buffers, add transfer destination flag for staging
        if (desc_.memory_type & BufferMemoryType::DeviceLocal)
        {
            usage_flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            usage_flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        std::vector<std::uint32_t> queue_families;
        queue_families.reserve(3);

        const auto families = device->queue_family_indices();
        queue_families.push_back(families.graphics_family);

        if (families.compute_family != families.graphics_family)
            queue_families.push_back(families.compute_family);

        if (families.transfer_family != families.graphics_family &&
            families.transfer_family != families.compute_family)
            queue_families.push_back(families.transfer_family);

        const bool concurrent_sharing = queue_families.size() > 1;

        const VkBufferCreateInfo buffer_create_info
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = desc_.size,
            .usage = usage_flags,
            .sharingMode = concurrent_sharing ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = concurrent_sharing ? static_cast<std::uint32_t>(queue_families.size()) : 0u,
            .pQueueFamilyIndices = concurrent_sharing ? queue_families.data() : nullptr,
        };

        VmaAllocationCreateInfo allocation_create_info{};

        const bool is_device_local  = desc_.memory_type & BufferMemoryType::DeviceLocal;
        const bool is_host_visible  = desc_.memory_type & BufferMemoryType::HostVisible;
        const bool is_host_coherent = desc_.memory_type & BufferMemoryType::HostCoherent;

        if (is_device_local && !is_host_visible && !is_host_coherent)
        {
            allocation_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        }
        else
        {
            allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            allocation_create_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

            if (is_host_coherent)
            {
                allocation_create_info.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
                allocation_create_info.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            }
        }

        const auto allocator = device->allocator()->vma_allocator();

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

        if ((desc_.memory_type & BufferMemoryType::HostVisible) &&
            !(desc_.memory_type & BufferMemoryType::HostCoherent))
        {
            if (!vk_check(
                vmaInvalidateAllocation(allocator, allocation_, 0, VK_WHOLE_SIZE),
                "Failed to invalidate host-visible buffer allocation"))
            {
                vmaUnmapMemory(allocator, allocation_);
                return nullptr;
            }
        }

        return mapped_data;
    }

    void Buffer::unmap()
    {
        const auto allocator = reinterpret_cast<Device*>(desc_.device)->allocator()->vma_allocator();

        if ((desc_.memory_type & BufferMemoryType::HostVisible) &&
            !(desc_.memory_type & BufferMemoryType::HostCoherent))
        {
            (void)vk_check(
                vmaFlushAllocation(allocator, allocation_, 0, VK_WHOLE_SIZE),
                "Failed to flush host-visible buffer allocation"
            );
        }

        vmaUnmapMemory(allocator, allocation_);
    }

    std::unique_ptr<rhi::Buffer> Buffer::stage(const size_t size) const
    {
        const size_t stage_size = size > 0 ? size : desc_.size;

        if (stage_size > desc_.size)
        {
            Log::error("Requested stage size {} exceeds buffer size {}", stage_size, desc_.size);
            return nullptr;
        }

        const BufferDesc staging_desc
        {
            .device = desc_.device,
            .size = stage_size,
            .usage = BufferUsage::Staging,
            .memory_type = BufferMemoryType::HostVisible | BufferMemoryType::HostCoherent
        };

        auto staging_buffer = create<Buffer>(staging_desc);
        if (!staging_buffer)
        {
            Log::error("Failed to create staging buffer");
            return nullptr;
        }

        if (desc_.memory_type & BufferMemoryType::DeviceLocal)
        {
            const auto* device = reinterpret_cast<Device*>(desc_.device);
            const std::uint32_t graphics_family = device->queue_family_indices().graphics_family;
            auto* command_pool = static_cast<CommandPool*>(device->command_pool(graphics_family));
            if (!command_pool)
            {
                Log::error("Failed to get graphics command pool for family {}", graphics_family);
                return nullptr;
            }

            auto* cmd_buffer = command_pool->begin_single_time_commands();
            if (!cmd_buffer)
            {
                Log::error("Failed to begin single time commands for buffer staging");
                return nullptr;
            }

            const VkBufferCopy copy_region
            {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = stage_size
            };

            const auto* vk_staging_buffer = static_cast<Buffer*>(staging_buffer.get());
            auto* vk_cmd = reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer();
            vkCmdCopyBuffer(vk_cmd, buffer_, vk_staging_buffer->vk_buffer(), 1, &copy_region);

            if (!command_pool->end_single_time_commands(cmd_buffer))
            {
                Log::error("Failed to end single time commands for buffer staging");
                return nullptr;
            }
        }
        else if (const auto mapped_data = const_cast<Buffer*>(this)->map())
        {
            staging_buffer->upload(mapped_data, stage_size, 0);
            const_cast<Buffer*>(this)->unmap();
        }
        else
        {
            Log::error("Failed to map source buffer for staging");
            return nullptr;
        }

        return staging_buffer;
    }

    void Buffer::upload(const void* data, const size_t size, const size_t offset)
    {
        assert(offset + size <= desc_.size, "Upload out of bounds");

        // For DeviceLocal buffers, use staging buffer with transfer queue
        if (desc_.memory_type & BufferMemoryType::DeviceLocal)
        {
            const BufferDesc staging_desc
            {
                .device = desc_.device,
                .size = size,
                .usage = BufferUsage::Staging,
                .memory_type = BufferMemoryType::HostVisible | BufferMemoryType::HostCoherent
            };

            const auto staging_buffer = create<Buffer>(staging_desc);
            if (!staging_buffer)
            {
                Log::error("Failed to create staging buffer");
                return;
            }

            staging_buffer->upload(data, size, 0);

            if (!upload_from(staging_buffer.get(), size, 0, offset))
            {
                Log::error("Failed to upload from staging buffer");
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
        assert(offset + size <= desc_.size, "Read back out of bounds");

        if (desc_.memory_type & BufferMemoryType::DeviceLocal)
        {
            const BufferDesc staging_desc
            {
                .device = desc_.device,
                .size = size,
                .usage = BufferUsage::Staging,
                .memory_type = BufferMemoryType::HostVisible | BufferMemoryType::HostCoherent
            };

            const auto staging_buffer = create<Buffer>(staging_desc);
            if (!staging_buffer)
            {
                Log::error("Failed to create staging buffer for buffer read-back");
                return;
            }

            const auto* device = reinterpret_cast<Device*>(desc_.device);
            const std::uint32_t graphics_family = device->queue_family_indices().graphics_family;
            auto* command_pool = static_cast<CommandPool*>(device->command_pool(graphics_family));
            if (!command_pool)
            {
                Log::error("Failed to get graphics command pool for family {}", graphics_family);
                return;
            }

            auto* cmd_buffer = command_pool->begin_single_time_commands();
            if (!cmd_buffer)
            {
                Log::error("Failed to begin single time commands for buffer read-back");
                return;
            }

            const VkBufferCopy copy_region
            {
                .srcOffset = offset,
                .dstOffset = 0,
                .size = size
            };

            const auto* vk_staging_buffer = static_cast<Buffer*>(staging_buffer.get());
            auto* vk_cmd = reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer();
            vkCmdCopyBuffer(vk_cmd, buffer_, vk_staging_buffer->vk_buffer(), 1, &copy_region);

            if (!command_pool->end_single_time_commands(cmd_buffer))
            {
                Log::error("Failed to end single time commands for buffer read-back");
                return;
            }

            staging_buffer->read_back(data, size, 0);
            return;
        }

        if (const auto mapped_data = map())
        {
            std::memcpy(data, static_cast<const char*>(mapped_data) + offset, size);
            unmap();
        }
    }

    bool Buffer::upload_from(
        rhi::Buffer* staging_buffer,
        const size_t size,
        const size_t src_offset,
        const size_t dst_offset)
    {
        if (!staging_buffer)
        {
            Log::error("Cannot upload from null staging buffer");
            return false;
        }

        const size_t source_size = staging_buffer->size();
        if (src_offset > source_size || dst_offset > desc_.size)
        {
            Log::error("Invalid source or destination offset for staging upload");
            return false;
        }

        const size_t transfer_size = size > 0 ? size : std::min(source_size - src_offset, desc_.size - dst_offset);

        if (transfer_size == 0) return true;

        if (src_offset + transfer_size > source_size || dst_offset + transfer_size > desc_.size)
        {
            Log::error("Staging upload out of bounds");
            return false;
        }

        if (!(desc_.memory_type & BufferMemoryType::DeviceLocal))
        {
            const auto src_data = staging_buffer->read_back(transfer_size, src_offset);
            upload(src_data.data(), transfer_size, dst_offset);
            return true;
        }

        const auto* device = reinterpret_cast<Device*>(desc_.device);
        const std::uint32_t graphics_family = device->queue_family_indices().graphics_family;
        auto* command_pool = static_cast<CommandPool*>(device->command_pool(graphics_family));
        if (!command_pool)
        {
            Log::error("Failed to get graphics command pool for family {}", graphics_family);
            return false;
        }

        auto* cmd_buffer = command_pool->begin_single_time_commands();
        if (!cmd_buffer)
        {
            Log::error("Failed to begin single time commands");
            return false;
        }

        const VkBufferCopy copy_region
        {
            .srcOffset = src_offset,
            .dstOffset = dst_offset,
            .size = transfer_size
        };

        const auto* vk_staging_buffer = static_cast<Buffer*>(staging_buffer);
        auto* vk_cmd = reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer();
        vkCmdCopyBuffer(vk_cmd, vk_staging_buffer->vk_buffer(), buffer_, 1, &copy_region);

        if (!command_pool->end_single_time_commands(cmd_buffer))
        {
            Log::error("Failed to end single time commands");
            return false;
        }

        return true;
    }

    VkBuffer Buffer::vk_buffer() const { return buffer_; }
}
