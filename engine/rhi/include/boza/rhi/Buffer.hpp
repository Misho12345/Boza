#pragma once
#include "boza/std_pch.hpp"
#include "GraphicsObject.hpp"

#include "Device.hpp"

namespace boza::rhi
{
    enum class BufferUsage : uint8_t
    {
        Vertex,
        Index,
        Uniform,
        Storage
    };

    enum class BufferMemoryType : uint8_t
    {
        DeviceLocal,
        HostVisible,
        HostCoherent
    };

    struct BufferDesc
    {
        Device*          device;
        size_t           size;
        BufferUsage      usage;
        BufferMemoryType memory_type;
    };

    class Buffer : public GraphicsObject<Buffer, BufferDesc>
    {
    public:
        virtual void* map() = 0;
        virtual void  unmap() = 0;

        [[nodiscard]]
        virtual size_t size() const = 0;
        virtual void   upload(const void* data, size_t size, size_t offset) = 0;

    protected:
        explicit Buffer(const BufferDesc& desc) : GraphicsObject(desc) {}
    };
}
