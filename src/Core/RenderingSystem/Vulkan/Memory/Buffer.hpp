#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class Buffer final
    {
    public:
        ~Buffer() = default;
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        [[nodiscard]]
        static Buffer create(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO);

        [[nodiscard]] static Buffer create_uniform_buffer(VkDeviceSize size);
        [[nodiscard]] static Buffer create_vertex_buffer(VkDeviceSize size);
        [[nodiscard]] static Buffer create_index_buffer(VkDeviceSize size);
        [[nodiscard]] static Buffer create_staging_buffer(VkDeviceSize size);
        [[nodiscard]] static Buffer create_storage_buffer(VkDeviceSize size);

        void destroy();

        void read(void* data, VkDeviceSize size) const;
        void write(const void* data, VkDeviceSize size, VkDeviceSize offset) const;

        [[nodiscard]] VkBuffer      get_buffer() const;
        [[nodiscard]] VmaAllocation get_allocation() const;

    private:
        Buffer() = default;

        VkBuffer buffer{ nullptr };
        VmaAllocation allocation{ nullptr };
        VmaAllocationInfo allocation_info{};
    };
}
