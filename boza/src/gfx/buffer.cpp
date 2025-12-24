module boza.gfx;

import :buffer;
import boza.rhi;
import boza.core;
import boza.detail;

namespace boza
{
    Buffer::Buffer(
        const std::size_t buffer_size,
        const BufferUsage usage,
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
            void* rhi_buffer = rhi::create_buffer(
                static_cast<rhi::GraphicsApi>(detail::RenderContext::api()), {
                    .device = static_cast<rhi::Device*>(detail::RenderContext::device()),
                    .size = buffer_size,
                    .usage = usage,
                    .memory_type = memory_type
                });

            if (!rhi_buffer)
            {
                Log::error("Failed to create buffer {} of {} (size {})", i, buffer_count, buffer_size);
                for (auto* buf : rhi_buffers_)
                {
                    auto* buffer = static_cast<rhi::Buffer*>(buf);
                    buffer->destroy();
                    delete buffer;
                }
                rhi_buffers_.clear();
                return;
            }

            rhi_buffers_.push_back(rhi_buffer);
        }
    }

    Buffer::~Buffer()
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

    Buffer::Buffer(Buffer&& other) noexcept
        : rhi_buffers_(std::move(other.rhi_buffers_)),
          size_(other.size_),
          access_mode_(other.access_mode_)
    {
        other.size_ = 0;
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this != &other)
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

            rhi_buffers_ = std::move(other.rhi_buffers_);
            size_ = other.size_;
            access_mode_ = other.access_mode_;

            other.size_ = 0;
        }
        return *this;
    }

    void Buffer::upload(const void* data, const std::size_t data_size, const std::size_t offset, const std::uint32_t frame_index)
    {
        if (rhi_buffers_.empty())
        {
            Log::error("Cannot upload to null buffer");
            return;
        }

        if (offset + data_size > size_)
        {
            Log::error("Buffer upload overflow: offset {} + size {} > buffer size {}", offset, data_size, size_);
            return;
        }

        const std::uint32_t buffer_index = access_mode_ == ResourceAccessMode::Dynamic ? frame_index : 0;

        if (buffer_index >= rhi_buffers_.size())
        {
            Log::error("Invalid frame index {} for buffer with {} buffers", frame_index, rhi_buffers_.size());
            return;
        }

        static_cast<rhi::Buffer*>(rhi_buffers_[buffer_index])->upload(data, data_size, offset);
    }

    void Buffer::read_back(void* data, const std::size_t data_size, const std::size_t offset, const std::uint32_t frame_index)
    {
        if (rhi_buffers_.empty())
        {
            Log::error("Cannot read from null buffer");
            return;
        }

        if (offset + data_size > size_)
        {
            Log::error("Buffer read overflow: offset {} + size {} > buffer size {}", offset, data_size, size_);
            return;
        }

        const std::uint32_t buffer_index = access_mode_ == ResourceAccessMode::Dynamic ? frame_index : 0;

        if (buffer_index >= rhi_buffers_.size())
        {
            Log::error("Invalid frame index {} for buffer with {} buffers", frame_index, rhi_buffers_.size());
            return;
        }

        static_cast<rhi::Buffer*>(rhi_buffers_[buffer_index])->read_back(data, data_size, offset);
    }

    void* Buffer::map(const std::uint32_t frame_index) const
    {
        if (rhi_buffers_.empty())
        {
            Log::error("Cannot map null buffer");
            return nullptr;
        }

        const std::uint32_t buffer_index = access_mode_ == ResourceAccessMode::Dynamic ? frame_index : 0;

        if (buffer_index >= rhi_buffers_.size())
        {
            Log::error("Invalid frame index {} for buffer with {} buffers", frame_index, rhi_buffers_.size());
            return nullptr;
        }

        return static_cast<rhi::Buffer*>(rhi_buffers_[buffer_index])->map();
    }

    void Buffer::unmap(const std::uint32_t frame_index) const
    {
        if (rhi_buffers_.empty())
        {
            Log::error("Cannot unmap null buffer");
            return;
        }

        const std::uint32_t buffer_index = access_mode_ == ResourceAccessMode::Dynamic ? frame_index : 0;

        if (buffer_index >= rhi_buffers_.size())
        {
            Log::error("Invalid frame index {} for buffer with {} buffers", frame_index, rhi_buffers_.size());
            return;
        }

        static_cast<rhi::Buffer*>(rhi_buffers_[buffer_index])->unmap();
    }

    void* Buffer::rhi_handle(const std::uint32_t frame_index) const
    {
        if (rhi_buffers_.empty())
        {
            return nullptr;
        }

        const std::uint32_t buffer_index = access_mode_ == ResourceAccessMode::Dynamic ? frame_index : 0;

        if (buffer_index >= rhi_buffers_.size())
        {
            Log::error("Invalid frame index {} for buffer with {} buffers", frame_index, rhi_buffers_.size());
            return nullptr;
        }

        return rhi_buffers_[buffer_index];
    }
}

