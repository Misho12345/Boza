module;

#include <cassert>

module boza.rhi.vulkan;

import :device;
import :resources;
import :util;

import <vk_all.h>;

namespace boza::rhi::vk
{
    VkBufferUsageFlags to_vk(const BufferUsage usage)
    {
        switch (usage)
        {
            case BufferUsage::Vertex: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            case BufferUsage::Index: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            case BufferUsage::Uniform: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            case BufferUsage::Storage: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            case BufferUsage::Staging: return VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        std::unreachable();
    }

    VmaMemoryUsage to_vma(const BufferMemoryType memory_type)
    {
        switch (memory_type)
        {
            case BufferMemoryType::DeviceLocal: return VMA_MEMORY_USAGE_GPU_ONLY;
            case BufferMemoryType::HostVisible: return VMA_MEMORY_USAGE_CPU_TO_GPU;
            case BufferMemoryType::HostCoherent: return VMA_MEMORY_USAGE_CPU_ONLY;
        }

        std::unreachable();
    }

    bool Buffer::init()
    {
        // Log::trace("Creating buffer ({} bytes)", desc.size);

        const VkBufferCreateInfo buffer_create_info
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = desc.size,
            .usage = to_vk(desc.usage),
        };

        VmaAllocationCreateInfo allocation_create_info{};
        allocation_create_info.usage = to_vma(desc.memory_type);

        const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();

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
        // Log::trace("Destroying buffer");

        if (buffer_)
        {
            const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();
            vmaDestroyBuffer(allocator, buffer_, allocation_);
        }
    }


    void* Buffer::map()
    {
        // Log::trace("Mapping buffer memory");

        void* mapped_data;
        const auto  allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();

        if (!vk_check(
            vmaMapMemory(allocator, allocation_, &mapped_data),
            "Failed to map buffer memory"))
            return nullptr;

        return mapped_data;
    }

    void Buffer::unmap()
    {
        // Log::trace("Unmapping buffer memory");

        const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();
        vmaUnmapMemory(allocator, allocation_);
    }

    size_t Buffer::size() const { return desc.size; }

    void Buffer::upload(const void* data, const size_t size, const size_t offset)
    {
        // Log::trace("Uploading {} bytes to buffer at offset {}", size, offset);

        assert(desc.memory_type != BufferMemoryType::DeviceLocal && "Cannot upload to device local buffer");
        assert(offset + size <= desc.size && "Upload out of bounds");

        if (const auto mapped_data = map())
        {
            std::memcpy(static_cast<char*>(mapped_data) + offset, data, size);
            unmap();
        }
    }

    void Buffer::read_back(void* data, const size_t size, const size_t offset)
    {
        // Log::trace("Reading back {} bytes from buffer at offset {}", size, offset);

        assert(desc.memory_type != BufferMemoryType::DeviceLocal && "Cannot read back from device local buffer");
        assert(offset + size <= desc.size && "Read back out of bounds");

        if (const auto mapped_data = map())
        {
            std::memcpy(data, static_cast<const char*>(mapped_data) + offset, size);
            unmap();
        }
    }

    VkBuffer Buffer::vk_buffer() const { return buffer_; }
}