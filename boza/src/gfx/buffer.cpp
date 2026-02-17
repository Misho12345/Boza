module boza.gfx;

import :buffer;

import boza.core;

import boza.rhi;
import boza.rhi.render_context;

namespace boza
{
    Buffer::Buffer(
        const std::size_t        buffer_size,
        const BufferUsage        usage,
        const ResourceAccessMode buffer_access_mode)
        : size_(buffer_size),
          access_mode_(buffer_access_mode)
    {
        if (!rhi::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return;
        }

        auto memory_type = rhi::BufferMemoryType::HostVisible;
        if (usage == BufferUsage::Vertex || usage == BufferUsage::Index)
        {
            memory_type = rhi::BufferMemoryType::DeviceLocal;
        }

        const std::uint32_t buffer_count = buffer_access_mode == ResourceAccessMode::Dynamic
                                               ? rhi::RenderContext::frames_in_flight()
                                               : 1;

        rhi_buffers_.reserve(buffer_count);

        for (std::uint32_t i = 0; i < buffer_count; ++i)
        {
            void* rhi_buffer = create_buffer(
                rhi::RenderContext::api(), {
                    .device = rhi::RenderContext::device(),
                    .size = buffer_size,
                    .usage = usage,
                    .memory_type = memory_type
                });

            if (!rhi_buffer)
            {
                Log::error("Failed to create buffer {} of {} (size {})", i, buffer_count, buffer_size);
                destroy();
                return;
            }

            rhi_buffers_.push_back(rhi_buffer);
        }
    }

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
        const void*         data,
        const std::size_t   data_size,
        const std::size_t   offset) const
    {
        if (auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer(offset, data_size)))
        {
            buffer->upload(data, data_size, offset);
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

    void* Buffer::map() const
    {
        if (auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer())) return buffer->map();
        return nullptr;
    }

    void Buffer::unmap() const
    {
        if (auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer())) buffer->unmap();
    }

    void* Buffer::rhi_handle() const
    {
        if (rhi_buffers_.empty()) return nullptr;

        const std::uint32_t frame_index = rhi::RenderContext::swapchain()->current_frame();
        const std::uint32_t buffer_index = access_mode_ == ResourceAccessMode::Dynamic ? frame_index : 0;

        if (buffer_index >= rhi_buffers_.size())
        {
            Log::error("Invalid frame index {} for buffer with {} buffers", frame_index, rhi_buffers_.size());
            return nullptr;
        }

        return rhi_buffers_[buffer_index];
    }

    void Buffer::destroy()
    {
        for (auto* rhi_buffer : rhi_buffers_)
        {
            if (rhi_buffer)
            {
                auto* buffer = static_cast<rhi::Buffer*>(rhi_buffer);
                buffer->destroy();
                delete buffer;
            }
        }
        rhi_buffers_.clear();
    }

    void* Buffer::get_validated_buffer(
        const std::size_t   offset,
        const std::size_t   data_size) const
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
