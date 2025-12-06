export module boza.rhi.objects:resources;

import std;
import boza.common;
import :graphics_object;

export namespace boza::rhi
{
    class Device;

    enum class ResourceAccessMode : std::uint8_t
    {
        Static,
        Dynamic
    };

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
        Device*            device;
        size_t             size;
        BufferUsage        usage;
        BufferMemoryType   memory_type;
        ResourceAccessMode access_mode = ResourceAccessMode::Static;
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

        mutable std::mutex resource_mutex_;
        std::atomic_bool in_use_{ false };
    };

    /// -------------------
    /// ===== Texture =====
    /// -------------------

    enum class TextureLayout : std::uint8_t
    {
        Undefined,
        General,
        ColorAttachment,
        DepthStencilAttachment,
        ShaderReadOnly,
        TransferSrc,
        TransferDst,
        Present
    };

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

    enum class DepthFormat : std::uint8_t
    {
        None = 0,
        D16,
        D24,
        D32F,
        D16S8,
        D24S8,
        D32FS8,
        Auto
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

    enum class TextureSampleCount : std::uint8_t
    {
        Count1  = 1,
        Count2  = 2,
        Count4  = 4,
        Count8  = 8,
        Count16 = 16,
        Count32 = 32,
        Count64 = 64
    };

    struct TextureDesc
    {
        Device* device;

        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t depth{ 1 };

        TextureFormat       format;
        Flags<TextureUsage> usage;
        ResourceAccessMode  access_mode{ ResourceAccessMode::Static };

        std::uint32_t       mip_levels{ 1 };
        std::uint32_t       array_layers{ 1 };
        TextureSampleCount  sample_count{ TextureSampleCount::Count1 };
    };

    class Texture : public GraphicsObject<Texture, TextureDesc>
    {
    public:
        virtual void upload(const void* data, size_t size) = 0;
        virtual void download(void* data, size_t size) = 0;
        virtual void transition_layout(TextureLayout old_layout, TextureLayout new_layout) = 0;

        [[nodiscard]] std::uint32_t width() const { return desc.width; }
        [[nodiscard]] std::uint32_t height() const { return desc.height; }

    protected:
        explicit Texture(const TextureDesc& desc) : GraphicsObject(desc) {}

        mutable std::mutex resource_mutex_;
        std::atomic<bool> in_use_{ false };
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
        ClampToBorder,
        Mirror
    };

    enum class SamplerMipmapMode : std::uint8_t
    {
        Nearest,
        Linear
    };

    enum class BorderColor : std::uint8_t
    {
        FloatTransparentBlack,
        IntTransparentBlack,
        FloatOpaqueBlack,
        IntOpaqueBlack,
        FloatOpaqueWhite,
        IntOpaqueWhite
    };

    enum class SamplerCompareOp : std::uint8_t
    {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    struct SamplerDesc
    {
        Device*            device;
        SamplerFilter      filter{ SamplerFilter::Linear };
        SamplerAddressMode address_mode_u{ SamplerAddressMode::Repeat };
        SamplerAddressMode address_mode_v{ SamplerAddressMode::Repeat };
        SamplerAddressMode address_mode_w{ SamplerAddressMode::Repeat };
        SamplerMipmapMode  mipmap_mode{ SamplerMipmapMode::Linear };
        float              mip_lod_bias{ 0.0f };
        float              min_lod{ 0.0f };
        float              max_lod{ 1000.0f };
        float              max_anisotropy{ 1.0f };
        bool               compare_enable{ false };
        SamplerCompareOp   compare_op{ SamplerCompareOp::Always };
        BorderColor        border_color{ BorderColor::IntOpaqueBlack };
        bool               unnormalized_coordinates{ false };
    };

    class Sampler : public GraphicsObject<Sampler, SamplerDesc>
    {
    protected:
        explicit Sampler(const SamplerDesc& desc) : GraphicsObject(desc) {}
    };
}
