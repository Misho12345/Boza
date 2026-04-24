export module boza.rhi.objects:resources;

import std;
import boza.common;
import boza.gfx.common;
import :graphics_object;

export namespace boza::rhi
{
    class Device;

    /// ------------------
    /// ===== Buffer =====
    /// ------------------

    enum class BufferMemoryType : std::uint8_t
    {
        None         = 0,
        DeviceLocal  = 1 << 0,
        HostVisible  = 1 << 1,
        HostCoherent = 1 << 2
    };

    constexpr Flags<BufferMemoryType> operator|(const BufferMemoryType left, const BufferMemoryType right) noexcept
    {
        return Flags(left) | Flags(right);
    }

    constexpr Flags<BufferMemoryType> operator&(const BufferMemoryType left, const BufferMemoryType right) noexcept
    {
        return Flags(left) & Flags(right);
    }

    constexpr Flags<BufferMemoryType> operator^(const BufferMemoryType left, const BufferMemoryType right) noexcept
    {
        return Flags(left) ^ Flags(right);
    }

    constexpr Flags<BufferMemoryType> operator~(const BufferMemoryType value) noexcept { return ~Flags(value); }

    [[nodiscard]]
    constexpr bool has_memory_type(const Flags<BufferMemoryType> memory_type, const BufferMemoryType flag) noexcept
    {
        return (memory_type & flag).any();
    }

    struct BufferDesc
    {
        Device*                device;
        std::size_t            size;
        BufferUsage            usage;
        Flags<BufferMemoryType> memory_type;
    };

    class Buffer : public GraphicsObject<Buffer, BufferDesc>
    {
    public:
        virtual void* map() = 0;
        virtual void  unmap() = 0;

        [[nodiscard]] virtual std::unique_ptr<Buffer> stage(std::size_t byte_size = 0) const = 0;
        [[nodiscard]] virtual std::size_t size() const { return desc_.size; }

        void upload(const std::span<const std::uint8_t> data, const std::size_t offset = 0)
        {
            upload(data.data(), data.size(), offset);
        }

        [[nodiscard]]
        std::vector<std::uint8_t> read_back(const std::size_t size, const std::size_t offset = 0)
        {
            std::vector<std::uint8_t> data(size);
            read_back(data.data(), size, offset);
            return data;
        }

        virtual void   upload(const void* data, std::size_t size, std::size_t offset) = 0;
        virtual void   read_back(void* data, std::size_t size, std::size_t offset) = 0;
        virtual bool   upload_from(Buffer* staging_buffer, std::size_t size = 0, std::size_t src_offset = 0, std::size_t dst_offset = 0) = 0;

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

        TextureType        type;
        TextureFormat      format;
        TextureSampleCount sample_count{ TextureSampleCount::Count1 };
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
        [[nodiscard]] virtual std::unique_ptr<Buffer> stage(std::size_t size = 0, std::uint32_t layer = 0) const = 0;

        void upload(const std::span<const std::uint8_t> data, const std::uint32_t layer = 0)
        {
            upload(data.data(), data.size(), layer);
        }

        [[nodiscard]] std::vector<std::uint8_t> read_back(const std::size_t size, const std::uint32_t layer = 0)
        {
            std::vector<std::uint8_t> data(size);
            read_back(data.data(), size, layer);
            return data;
        }

        virtual void upload(const void* data, std::size_t size, std::uint32_t layer) = 0;
        virtual void read_back(void* data, std::size_t size, std::uint32_t layer) = 0;
        virtual bool upload_from(Buffer* staging_buffer, std::size_t size = 0, std::uint32_t layer = 0) = 0;
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
