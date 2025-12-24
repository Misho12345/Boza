module;

#include <cstddef>
#include "api.hpp"

export module boza.gfx:texture;

import std;
import boza.common;
import :common;

export namespace boza
{
    enum class TextureLayout
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

    enum class TextureFormat
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

    enum class TextureUsage
    {
        Sampled                = 1 << 0,
        Storage                = 1 << 1,
        ColorAttachment        = 1 << 2,
        DepthStencilAttachment = 1 << 3,
        TransferSrc            = 1 << 4,
        TransferDst            = 1 << 5,
        InputAttachment        = 1 << 6
    };

    constexpr Flags<TextureUsage> operator|(const TextureUsage left, const TextureUsage right) noexcept
    {
        return Flags(left) | Flags(right);
    }

    constexpr Flags<TextureUsage> operator&(const TextureUsage left, const TextureUsage right) noexcept
    {
        return Flags(left) & Flags(right);
    }

    constexpr Flags<TextureUsage> operator^(const TextureUsage left, const TextureUsage right) noexcept
    {
        return Flags(left) ^ Flags(right);
    }

    constexpr Flags<TextureUsage> operator~(const TextureUsage value) noexcept { return ~Flags(value); }

    enum class SamplerFilter
    {
        Nearest,
        Linear,
        Anisotropic
    };

    enum class SamplerWrap
    {
        Repeat,
        ClampToEdge,
        ClampToBorder,
        Mirror
    };

    class BOZA_API Texture
    {
    public:
        Texture(
            std::uint32_t       width,
            std::uint32_t       height,
            TextureFormat       format,
            Flags<TextureUsage> usage_flags,
            ResourceAccessMode  access_mode = ResourceAccessMode::Static,
            SamplerFilter       filter      = SamplerFilter::Linear,
            SamplerWrap         wrap        = SamplerWrap::Repeat);

        ~Texture();

        Texture(const Texture&)            = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(Texture&&) noexcept;

        static Texture* get_or_load(
            const std::string& filepath,
            TextureFormat      texture_format = TextureFormat::RGBA8,
            SamplerFilter      filter         = SamplerFilter::Linear,
            SamplerWrap        wrap           = SamplerWrap::Repeat);

        static Texture* load_copy(
            const std::string& filepath,
            TextureFormat      texture_format      = TextureFormat::RGBA8,
            ResourceAccessMode texture_access_mode = ResourceAccessMode::Static,
            SamplerFilter      filter              = SamplerFilter::Linear,
            SamplerWrap        wrap                = SamplerWrap::Repeat);

        static Texture* create(
            const std::string&  name,
            std::uint32_t       texture_width,
            std::uint32_t       texture_height,
            TextureFormat       texture_format,
            Flags<TextureUsage> usage_flags,
            ResourceAccessMode  texture_access_mode = ResourceAccessMode::Static,
            SamplerFilter       filter              = SamplerFilter::Linear,
            SamplerWrap         wrap                = SamplerWrap::Repeat);

        static Texture* get(const std::string& name);
        static void     destroy(Texture* texture);

        void upload(const void* data, std::size_t data_size, std::uint32_t frame_index = 0) const;
        bool save_to_file(const std::string& filepath, std::uint32_t frame_index = 0) const;

        void transition_layout(TextureLayout old_layout, TextureLayout new_layout, std::uint32_t frame_index = 0) const;

        void change_sampler(const SamplerFilter filter) { change_sampler(filter, wrap_); }
        void change_sampler(const SamplerWrap wrap) { change_sampler(filter_, wrap); }
        void change_sampler(SamplerFilter filter, SamplerWrap wrap);

        PropertyGet<Texture, std::uint32_t> width{ &Texture::get_width, offsetof(Texture, width) };
        PropertyGet<Texture, std::uint32_t> height{ &Texture::get_height, offsetof(Texture, height) };
        PropertyGet<Texture, TextureFormat> format{ &Texture::get_format, offsetof(Texture, format) };

        PropertyGet<Texture, ResourceAccessMode> access_mode
        {
            &Texture::get_access_mode,
            offsetof(Texture, access_mode)
        };

        [[nodiscard]] void* rhi_handle(std::uint32_t frame_index = 0) const;
        [[nodiscard]] void* rhi_sampler_handle() const { return rhi_sampler_; }
        [[nodiscard]] bool  is_valid() const { return !rhi_textures_.empty() && rhi_sampler_; }

    private:
        [[nodiscard]] std::uint32_t      get_width() const { return width_; }
        [[nodiscard]] std::uint32_t      get_height() const { return height_; }
        [[nodiscard]] TextureFormat      get_format() const { return format_; }
        [[nodiscard]] ResourceAccessMode get_access_mode() const { return access_mode_; }

        [[nodiscard]] std::uint32_t resolve_texture_index(std::uint32_t frame_index) const;

        static void* create_rhi_sampler(SamplerFilter filter, SamplerWrap wrap);
        void         cleanup_textures();
        void         cleanup_sampler();

        std::vector<void*> rhi_textures_{};
        void*              rhi_sampler_{ nullptr };

        std::uint32_t width_;
        std::uint32_t height_;
        TextureFormat format_;

        SamplerFilter filter_;
        SamplerWrap   wrap_;

        ResourceAccessMode access_mode_;
    };
}
