export module boza.rhi.objects:resources;

import std;
import boza.common;
import :graphics_object;

export namespace boza::rhi
{
    class Device;

    /// ------------------
    /// ===== Buffer =====
    /// ------------------

    enum class BufferUsage : std::uint8_t
    {
        Vertex,
        Index,
        Uniform,
        Storage,
        Staging
    };

    enum class BufferMemoryType : std::uint8_t
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
        virtual void   read_back(void* data, size_t size, size_t offset) = 0;

    protected:
        explicit Buffer(const BufferDesc& desc) : GraphicsObject(desc) {}
    };

    /// -------------------
    /// ===== Texture =====
    /// -------------------

    enum class TextureFormat : std::uint8_t
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

    enum class TextureUsage : std::uint8_t
    {
        Sampled                = 1 << 0,
        Storage                = 1 << 1,
        ColorAttachment        = 1 << 2,
        DepthStencilAttachment = 1 << 3,
        TransferSrc            = 1 << 4,
        TransferDst            = 1 << 5,
        InputAttachment        = 1 << 6
    };

    struct TextureDesc
    {
        Device* device;

        std::uint32_t width;
        std::uint32_t height;

        TextureFormat       format;
        Flags<TextureUsage> usage;
    };

    class Texture : public GraphicsObject<Texture, TextureDesc>
    {
    public:
        virtual void upload(const void* data, size_t size) = 0;
        virtual bool load_from_file(const std::string& filepath) = 0;

        virtual bool save_to_file(const std::string& filepath) = 0;
        virtual void transition_layout_external() = 0;

        [[nodiscard]] std::uint32_t width() const { return desc.width; }
        [[nodiscard]] std::uint32_t height() const { return desc.height; }

    protected:
        explicit Texture(const TextureDesc& desc) : GraphicsObject(desc) {}
    };


    /// -------------------
    /// ===== Sampler =====
    /// -------------------

    enum class SamplerFilter : std::uint8_t
    {
        Nearest,
        Linear,
        Anisotropic
    };

    enum class SamplerAddressMode : std::uint8_t
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
