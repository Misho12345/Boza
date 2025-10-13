#pragma once
#include "boza/pch.hpp"
#include "GraphicsObject.hpp"

namespace boza::rhi
{
    class Device;

    /// ------------------
    /// ===== Buffer =====
    /// ------------------

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
        bool             is_constant = false; // If true, create 1 buffer; if false, create per-swapchain-image buffers
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

    /// -------------------
    /// ===== Texture =====
    /// -------------------

    enum class TextureFormat : uint8_t
    {
        R8,
        RG8,
        RGB8,
        RGBA8,
        BGRA8,
        R16F,
        RG16F,
        RGB16F,
        RGBA16F,
        R32F,
        RG32F,
        RGB32F,
        RGBA32F,
        DEPTH24STENCIL8,
        DEPTH32F
    };

    enum class TextureUsage : uint8_t
    {
        Sampled,
        Storage,
        ColorAttachment,
        DepthStencilAttachment,
        TransferSrc,
        TransferDst,
        InputAttachment
    };

    struct TextureDesc
    {
        Device* device;

        uint32_t width;
        uint32_t height;

        TextureFormat format;
        TextureUsage  usage;
    };

    class Texture : public GraphicsObject<Texture, TextureDesc>
    {
    public:
        virtual void upload(const void* data, size_t size) = 0;
        virtual bool load_from_file(const std::string& filepath) = 0;

        [[nodiscard]] uint32_t width() const { return desc.width; }
        [[nodiscard]] uint32_t height() const { return desc.height; }

    protected:
        explicit Texture(const TextureDesc& desc) : GraphicsObject(desc) {}
    };


    /// -------------------
    /// ===== Sampler =====
    /// -------------------

    enum class SamplerFilter : uint8_t
    {
        Nearest,
        Linear,
        Anisotropic
    };

    enum class SamplerAddressMode : uint8_t
    {
        Repeat,
        ClampToEdge,
        Mirror
    };

    struct SamplerDesc
    {
        Device*            device;
        SamplerFilter      filter;
        SamplerAddressMode address_mode_u = SamplerAddressMode::Repeat;
        SamplerAddressMode address_mode_v = SamplerAddressMode::Repeat;
        SamplerAddressMode address_mode_w = SamplerAddressMode::Repeat;
    };

    class Sampler : public GraphicsObject<Sampler, SamplerDesc>
    {
    protected:
        explicit Sampler(const SamplerDesc& desc) : GraphicsObject(desc) {}
    };
}
