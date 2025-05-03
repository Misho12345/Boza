#include "Buffer.hpp"

#include "Allocator.hpp"
#include "Logger.hpp"

namespace boza
{
    Buffer::Buffer(Buffer&& other) noexcept
    {
        buffer = std::exchange(other.buffer, nullptr);
        allocation = std::exchange(other.allocation, nullptr);
        allocation_info = std::exchange(other.allocation_info, {});
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this != &other)
        {
            buffer = std::exchange(other.buffer, nullptr);
            allocation = std::exchange(other.allocation, nullptr);
            allocation_info = std::exchange(other.allocation_info, {});
        }

        return *this;
    }

    Buffer Buffer::create(
        const VkDeviceSize       size,
        const VkBufferUsageFlags usage,
        const VmaMemoryUsage     memory_usage)
    {
        Buffer buffer;

        const VkBufferCreateInfo buffer_info
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = size,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        const VmaAllocationCreateInfo allocation_info
        {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = memory_usage,
            .requiredFlags = {},
            .preferredFlags = {},
            .memoryTypeBits = {},
            .pool = {},
            .pUserData = nullptr,
            .priority = {}
        };

        VK_CHECK(vmaCreateBuffer(Allocator::get_vma_allocator(), &buffer_info, &allocation_info,
                     &buffer.buffer, &buffer.allocation, &buffer.allocation_info),
        {
            LOG_VK_ERROR("Failed to create buffer");
            return {};
        });

        return buffer;
    }


    Buffer Buffer::create_uniform_buffer(const VkDeviceSize size) { return create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU); }
    Buffer Buffer::create_vertex_buffer(const VkDeviceSize size) { return create(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU); }
    Buffer Buffer::create_index_buffer(const VkDeviceSize size) { return create(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU); }
    Buffer Buffer::create_staging_buffer(const VkDeviceSize size) { return create(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY); }
    Buffer Buffer::create_storage_buffer(const VkDeviceSize size) { return create(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY); }

    VkBuffer      Buffer::get_buffer() const { return buffer; }
    VmaAllocation Buffer::get_allocation() const { return allocation; }


    void Buffer::destroy()
    {
        if (buffer != nullptr)
        {
            vmaDestroyBuffer(Allocator::get_vma_allocator(), buffer, allocation);
            buffer     = nullptr;
            allocation = nullptr;
        }
    }


    void Buffer::read(void* data, const VkDeviceSize size) const
    {
        memcpy(data, allocation_info.pMappedData, size);
    }

    void Buffer::write(const void* data, const VkDeviceSize size, const VkDeviceSize offset) const
    {
        memcpy(allocation_info.pMappedData, static_cast<const char*>(data) + offset, size);
    }
}
