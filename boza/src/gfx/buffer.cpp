module boza.gfx;

import :buffer;

import boza.core;
import boza.rhi;
import boza.rhi.render_context;

namespace boza
{
    void* BufferAccess::handle(const Buffer& buffer) { return buffer.rhi_handle(); }

    void* BufferAccess::handle(const Buffer& buffer, const std::uint32_t frame_index)
    {
        return buffer.rhi_handle(frame_index);
    }

    Buffer::Buffer(
        const std::size_t        buffer_size,
        const BufferUsage        usage,
        const ResourceAccessMode buffer_access_mode)
        : size_{ buffer_size },
          access_mode_{ buffer_access_mode }
    {
        if (!rhi::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return;
        }

        Flags memory_type{ rhi::BufferMemoryType::HostVisible | rhi::BufferMemoryType::HostCoherent };

        if (usage == BufferUsage::Vertex || usage == BufferUsage::Index)
        {
            memory_type = rhi::BufferMemoryType::DeviceLocal;
        }
        else if ((usage == BufferUsage::Uniform || usage == BufferUsage::Storage) &&
                 buffer_access_mode == ResourceAccessMode::Static)
        {
            memory_type = rhi::BufferMemoryType::DeviceLocal;
        }
        else if (usage == BufferUsage::Staging)
        {
            memory_type = rhi::BufferMemoryType::HostVisible | rhi::BufferMemoryType::HostCoherent;
        }

        const std::uint32_t buffer_count = buffer_access_mode == ResourceAccessMode::Dynamic
                                               ? rhi::RenderContext::frames_in_flight()
                                               : 1;

        rhi_buffers_.reserve(buffer_count);

        for (std::uint32_t i = 0; i < buffer_count; ++i)
        {
            auto rhi_buffer = create_buffer(
                rhi::RenderContext::api(), {
                    .device      = rhi::RenderContext::device(),
                    .size        = buffer_size,
                    .usage       = usage,
                    .memory_type = memory_type
                });

            if (!rhi_buffer)
            {
                Log::error("Failed to create buffer {} of {} (size {})", i, buffer_count, buffer_size);
                destroy();
                return;
            }

            rhi_buffers_.emplace_back(rhi_buffer.release(), &Buffer::destroy_rhi_buffer);
        }
    }

    Buffer::Buffer(
        std::vector<RhiBufferHandle>&& rhi_buffers,
        const std::size_t            buffer_size,
        const ResourceAccessMode     buffer_access_mode)
        : rhi_buffers_{ std::move(rhi_buffers) },
          size_{ buffer_size },
          access_mode_{ buffer_access_mode } {}

    Buffer::~Buffer() { destroy(); }

    Buffer::Buffer(Buffer&& other) noexcept
        : rhi_buffers_{ std::move(other.rhi_buffers_) },
          size_{ std::exchange(other.size_, 0) },
          access_mode_{ other.access_mode_ } {}

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this == &other) return *this;

        destroy();

        rhi_buffers_ = std::move(other.rhi_buffers_);
        size_        = std::exchange(other.size_, 0);
        access_mode_ = other.access_mode_;

        return *this;
    }

    void Buffer::upload(
        const void*       data,
        const std::size_t data_size,
        const std::size_t offset)
    {
        if (auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer(offset, data_size)))
        {
            buffer->upload(data, data_size, offset);
        }
    }

    void Buffer::upload_from(
        const Buffer&      staging_buffer,
        const std::size_t  byte_size,
        const std::size_t  src_offset,
        const std::size_t  dst_offset)
    {
        if (src_offset > staging_buffer.size_ || dst_offset > size_)
        {
            Log::error("Invalid source or destination offset for staging upload");
            return;
        }

        auto* dst = static_cast<rhi::Buffer*>(get_validated_buffer(dst_offset, byte_size));
        auto* src = static_cast<rhi::Buffer*>(staging_buffer.get_validated_buffer(src_offset, byte_size));

        if (!dst || !src) return;

        const std::size_t transfer_size =
            byte_size > 0
                ? byte_size
                : std::min(staging_buffer.size_ - src_offset, size_ - dst_offset);

        if (!dst->upload_from(src, transfer_size, src_offset, dst_offset))
        {
            Log::error("Failed to upload from staging buffer");
        }
    }

    void Buffer::read_back(
        void*             data,
        const std::size_t data_size,
        const std::size_t offset) const
    {
        if (auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer(offset, data_size)))
        {
            buffer->read_back(data, data_size, offset);
        }
    }

    std::vector<std::uint8_t> Buffer::read_back() const
    {
        std::vector<std::uint8_t> result(size_);
        read_back(result.data(), size_);
        return result;
    }

    Buffer Buffer::stage(const std::size_t byte_size) const
    {
        const std::size_t stage_size = byte_size > 0 ? byte_size : size_;

        if (rhi_buffers_.empty())
        {
            Log::error("Cannot stage an invalid buffer");
            return Buffer{
                stage_size,
                BufferUsage::Staging,
                ResourceAccessMode::Static
            };
        }

        std::vector<RhiBufferHandle> staged_rhi_buffers;
        staged_rhi_buffers.reserve(rhi_buffers_.size());

        for (const auto& handle : rhi_buffers_)
        {
            const auto* source_buffer = static_cast<rhi::Buffer*>(handle.get());
            if (!source_buffer)
            {
                Log::error("Cannot stage invalid buffer");
                staged_rhi_buffers.clear();
                break;
            }

            auto staged = source_buffer->stage(stage_size);
            if (!staged)
            {
                Log::error("Failed to stage buffer data");
                staged_rhi_buffers.clear();
                break;
            }

            staged_rhi_buffers.emplace_back(staged.release(), &Buffer::destroy_rhi_buffer);
        }

        if (staged_rhi_buffers.size() == rhi_buffers_.size())
        {
            return Buffer{
                std::move(staged_rhi_buffers),
                stage_size,
                access_mode_
            };
        }

        Buffer fallback_staging_buffer{
            stage_size,
            BufferUsage::Staging,
            ResourceAccessMode::Static
        };

        if (void* mapped_data = fallback_staging_buffer.map())
        {
            read_back(mapped_data, fallback_staging_buffer.size_);
            fallback_staging_buffer.unmap();
        }

        return fallback_staging_buffer;
    }

    void* Buffer::map()
    {
        if (auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer())) return buffer->map();
        return nullptr;
    }

    void Buffer::unmap()
    {
        if (auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer())) buffer->unmap();
    }

    void* Buffer::rhi_handle() const
    {
        if (rhi_buffers_.empty()) return nullptr;

        if (access_mode_ != ResourceAccessMode::Dynamic) return rhi_buffers_.front().get();

        const auto* swapchain = rhi::RenderContext::swapchain();
        if (!swapchain)
        {
            Log::error("Cannot resolve dynamic buffer handle: swapchain is null");
            return nullptr;
        }

        return rhi_handle(swapchain->current_frame());
    }

    void* Buffer::rhi_handle(const std::uint32_t frame_index) const
    {
        if (rhi_buffers_.empty()) return nullptr;
        if (access_mode_ != ResourceAccessMode::Dynamic) return rhi_buffers_.front().get();

        if (frame_index >= rhi_buffers_.size())
        {
            Log::error("Invalid frame index {} for buffer with {} buffers", frame_index, rhi_buffers_.size());
            return nullptr;
        }

        return rhi_buffers_[frame_index].get();
    }

    void Buffer::destroy()
    {
        rhi_buffers_.clear();
    }

    void Buffer::destroy_rhi_buffer(void* handle)
    {
        if (!handle) return;

        delete static_cast<rhi::Buffer*>(handle);
    }

    void* Buffer::get_validated_buffer(
        const std::size_t offset,
        const std::size_t data_size) const
    {
        if (rhi_buffers_.empty())
        {
            Log::error("Cannot access null buffer");
            return nullptr;
        }

        if (data_size > 0 && offset + data_size > size_)
        {
            Log::error("Buffer access overflow: offset {} + size {} > buffer size {}", offset, data_size, size_);
            return nullptr;
        }

        return rhi_handle();
    }
}
