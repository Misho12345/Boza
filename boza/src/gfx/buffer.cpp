module boza.gfx;

import :buffer;
import boza.rhi;
import boza.core;
import boza.detail;

namespace boza
{
    Buffer::Buffer(
        const std::size_t        buffer_size,
        const BufferUsage        usage,
        const ResourceAccessMode buffer_access_mode)
        : size_(buffer_size),
          access_mode_(buffer_access_mode)
    {
        if (!detail::RenderContext::initialized())
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
                                               ? detail::RenderContext::frames_in_flight()
                                               : 1;

        rhi_buffers_.reserve(buffer_count);

        for (std::uint32_t i = 0; i < buffer_count; ++i)
        {
            void* rhi_buffer = create_buffer(
                static_cast<rhi::GraphicsApi>(detail::RenderContext::api()), {
                    .device = static_cast<rhi::Device*>(detail::RenderContext::device()),
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
        : rhi_buffers_(std::move(other.rhi_buffers_)),
          size_(other.size_),
          access_mode_(other.access_mode_) { other.size_ = 0; }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this != &other)
        {
            destroy();

            rhi_buffers_ = std::move(other.rhi_buffers_);
            size_        = other.size_;
            access_mode_ = other.access_mode_;

            other.size_ = 0;
        }
        return *this;
    }

    void Buffer::upload(
        const void*         data,
        const std::size_t   data_size,
        const std::size_t   offset,
        const std::uint32_t frame_index) const
    {
        auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer(frame_index, offset, data_size));
        if (!buffer) return;

        buffer->upload(data, data_size, offset);
    }

    void Buffer::read_back(
        void*               data,
        const std::size_t   data_size,
        const std::size_t   offset,
        const std::uint32_t frame_index) const
    {
        auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer(frame_index, offset, data_size));
        if (!buffer) return;

        buffer->read_back(data, data_size, offset);
    }

    void* Buffer::map(const std::uint32_t frame_index) const
    {
        auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer(frame_index));
        if (!buffer) return nullptr;

        return buffer->map();
    }

    void Buffer::unmap(const std::uint32_t frame_index) const
    {
        auto* buffer = static_cast<rhi::Buffer*>(get_validated_buffer(frame_index));
        if (!buffer) return;

        buffer->unmap();
    }

    void* Buffer::rhi_handle(const std::uint32_t frame_index) const
    {
        if (rhi_buffers_.empty()) { return nullptr; }

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
        const std::uint32_t frame_index,
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

        const std::uint32_t buffer_index = access_mode_ == ResourceAccessMode::Dynamic ? frame_index : 0;

        if (buffer_index >= rhi_buffers_.size())
        {
            Log::error("Invalid frame index {} for buffer with {} buffers", frame_index, rhi_buffers_.size());
            return nullptr;
        }

        return static_cast<rhi::Buffer*>(rhi_buffers_[buffer_index]);
    }
}
