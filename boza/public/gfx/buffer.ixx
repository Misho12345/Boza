module;

#include <cstddef>
#include "api.hpp"

export module boza.gfx:buffer;

import :common;

import std;
import boza.common;

export namespace boza
{
    enum class BufferUsage
    {
        Vertex,
        Index,
        Uniform,
        Storage,
        Staging
    };

    class BOZA_API Buffer
    {
    public:
        Buffer(
            std::size_t buffer_size,
            BufferUsage usage,
            ResourceAccessMode buffer_access_mode = ResourceAccessMode::Static);

        ~Buffer();

        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept;
        Buffer& operator=(Buffer&&) noexcept;

        void upload(const void* data, std::size_t data_size, std::size_t offset = 0, std::uint32_t frame_index = 0);
        void read_back(void* data, std::size_t data_size, std::size_t offset = 0, std::uint32_t frame_index = 0);

        template<typename T>
        void upload(const T& data, const std::uint32_t frame_index = 0)
        {
            upload(&data, sizeof(T), 0, frame_index);
        }

        template<typename T>
        void upload(const std::vector<T>& data, const std::uint32_t frame_index = 0)
        {
            upload(data.data(), data.size() * sizeof(T), 0, frame_index);
        }

        PropertyGet<Buffer, std::size_t> size{ &Buffer::get_size, offsetof(Buffer, size) };
        PropertyGet<Buffer, ResourceAccessMode> access_mode{ &Buffer::get_access_mode, offsetof(Buffer, access_mode) };

        void* map(std::uint32_t frame_index = 0) const;
        void  unmap(std::uint32_t frame_index = 0) const;

        void* rhi_handle(std::uint32_t frame_index = 0) const;

    private:
        [[nodiscard]] std::size_t get_size() const { return size_; }
        [[nodiscard]] ResourceAccessMode get_access_mode() const { return access_mode_; }

        std::vector<void*> rhi_buffers_;
        std::size_t size_;
        ResourceAccessMode access_mode_;
    };
}
