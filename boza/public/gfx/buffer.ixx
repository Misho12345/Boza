module;

#include "api.hpp"

export module boza.gfx:buffer;

import :common;

import std;
import boza.common;

export namespace boza
{
    enum class BufferUsage : std::uint8_t
    {
        Vertex,
        Index,
        Uniform,
        Storage,
        Staging
    };

    class BOZA_API Buffer final
    {
        [[nodiscard]] std::size_t get_size() const { return size_; }
        [[nodiscard]] ResourceAccessMode get_access_mode() const { return access_mode_; }

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

        void upload(const void* data, std::size_t data_size, std::size_t offset = 0, std::uint32_t frame_index = 0) const;
        void read_back(void* data, std::size_t data_size, std::size_t offset = 0, std::uint32_t frame_index = 0) const;

        template<typename T>
        void upload(const T& data, const std::uint32_t frame_index = 0)
        {
            upload(&data, sizeof(T), 0, frame_index);
        }

        template<typename T>
        void upload(std::span<T> data, const std::uint32_t frame_index = 0)
        {
            upload(data.data(), data.size() * sizeof(T), 0, frame_index);
        }

        [[msvc::no_unique_address]] Property<Buffer, &Buffer::get_size> size{ this };
        [[msvc::no_unique_address]] Property<Buffer, &Buffer::get_access_mode> access_mode{ this };

        [[nodiscard]]
        void* map(std::uint32_t frame_index = 0) const;
        void  unmap(std::uint32_t frame_index = 0) const;

        [[nodiscard]] void* rhi_handle(std::uint32_t frame_index = 0) const;

    private:
        void destroy();
        [[nodiscard]]
        void* get_validated_buffer(
            std::uint32_t frame_index,
            std::size_t offset = 0,
            std::size_t data_size = 0) const;

        std::vector<void*> rhi_buffers_;
        std::size_t size_;
        ResourceAccessMode access_mode_;
    };
}
