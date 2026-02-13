export module boza.rhi.objects:resources;

import std;
import boza.common;
import boza.gfx;
import :graphics_object;

export namespace boza::rhi
{
    class Device;

    /// ------------------
    /// ===== Buffer =====
    /// ------------------

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

        TextureType         type;
        TextureFormat       format;
        TextureSampleCount  sample_count{ TextureSampleCount::Count1 };
        Flags<TextureUsage> usage;

        std::uint32_t width{ 1 };
        std::uint32_t height{ 1 };
        std::uint32_t depth{ 1 };

        std::uint32_t mip_levels{ 1 };
        std::uint32_t array_layers{ 1 };
    };

    class Texture : public GraphicsObject<Texture, TextureDesc>
    {
    public:
        virtual void upload(const void* data, size_t size, std::uint32_t layer) = 0;
        virtual void read_back(void* data, size_t size, std::uint32_t layer) = 0;
        virtual void transition_layout(TextureLayout old_layout, TextureLayout new_layout) = 0;

        [[nodiscard]] std::uint32_t width() const { return desc_.width; }
        [[nodiscard]] std::uint32_t height() const { return desc_.height; }

    protected:
        explicit Texture(const TextureDesc& desc) : GraphicsObject(desc) {}

        mutable std::mutex resource_mutex_;
        std::atomic_bool in_use_{ false };
    };


    /// -------------------
    /// ===== Sampler =====
    /// -------------------

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
        Device* device;

        TextureType type;

        SamplerFilter filter{ SamplerFilter::Linear };
        SamplerWrap   wrap_u{ SamplerWrap::Repeat };
        SamplerWrap   wrap_v{ SamplerWrap::Repeat };
        SamplerWrap   wrap_w{ SamplerWrap::Repeat };

        SamplerFilter mipmap_mode{ SamplerFilter::Linear };
        float         mip_lod_bias{ 0.0f };

        float min_lod{ 0.0f };
        float max_lod{ 1000.0f };
        float max_anisotropy{ 1.0f };

        bool             compare_enable{ false };
        SamplerCompareOp compare_op{ SamplerCompareOp::Always };

        BorderColor border_color{ BorderColor::IntOpaqueBlack };
        bool        unnormalized_coordinates{ false };
    };

    class Sampler : public GraphicsObject<Sampler, SamplerDesc>
    {
    protected:
        explicit Sampler(const SamplerDesc& desc) : GraphicsObject(desc) {}
    };
}
