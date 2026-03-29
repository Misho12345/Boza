module;

#include "api.hpp"

export module boza.gfx:buffer;

import std;
import boza.common;
import boza.gfx.common;

namespace boza
{
    class Buffer;

    struct BufferAccess
    {
        static void* handle(const Buffer& buffer);
        static void* handle(const Buffer& buffer, std::uint32_t frame_index);
    };
}

export namespace boza
{
    class Material;
    class Texture;

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

        void upload(const void* data, std::size_t data_size, std::size_t offset = 0);
        void upload_from(
            const Buffer& staging_buffer,
            std::size_t   byte_size = 0,
            std::size_t   src_offset = 0,
            std::size_t   dst_offset = 0);
        void read_back(void* data, std::size_t data_size, std::size_t offset = 0) const;

        template <typename T>
        void upload(const T& data) { upload(&data, sizeof(T), 0); }

        template <typename T>
        void upload(std::span<T> data) { upload(data.data(), data.size() * sizeof(T), 0); }

        [[nodiscard]] Buffer stage(std::size_t byte_size = 0) const;

        std::vector<std::uint8_t> read_back() const;

        [[msvc::no_unique_address]] Property<Buffer, &Buffer::get_size> size{ this };
        [[msvc::no_unique_address]] Property<Buffer, &Buffer::get_access_mode> access_mode{ this };

        [[nodiscard]]
        void* map();
        void  unmap();

    private:
        using RhiBufferHandle = std::unique_ptr<void, void(*)(void*)>;

        [[nodiscard]] void* rhi_handle() const;
        [[nodiscard]] void* rhi_handle(std::uint32_t frame_index) const;

        static void destroy_rhi_buffer(void* handle);
        void destroy();

        [[nodiscard]]
        void* get_validated_buffer(
            std::size_t offset = 0,
            std::size_t data_size = 0) const;

        Buffer(
            std::vector<RhiBufferHandle>&& rhi_buffers,
            std::size_t                    buffer_size,
            ResourceAccessMode             buffer_access_mode);

        std::vector<RhiBufferHandle> rhi_buffers_;
        std::size_t size_;
        ResourceAccessMode access_mode_;

        friend struct BufferAccess;
        friend class Material;
        friend class ComputeDispatcher;
        friend class Texture;
    };
}
