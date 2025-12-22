module;

#include <cstddef>
#include "api.hpp"

export module boza.gfx:texture;

import std;
import boza.common;

export namespace boza
{
    enum class TextureAccessMode
    {
        Static,
        Dynamic
    };

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
        Mirror
    };

    class BOZA_API Texture
    {
    public:
        Texture(
            std::uint32_t     texture_width,
            std::uint32_t     texture_height,
            TextureFormat     texture_format,
            Flags<TextureUsage> usage_flags,
            TextureAccessMode texture_access_mode = TextureAccessMode::Static);

        ~Texture();

        Texture(const Texture&)            = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(Texture&&) noexcept;

        static Texture* load_from_file(
            const std::string& filepath,
            TextureFormat      texture_format = TextureFormat::RGBA8,
            TextureAccessMode  texture_access_mode = TextureAccessMode::Static);

        static Texture* load(
            const std::string& filepath,
            TextureFormat      texture_format = TextureFormat::RGBA8,
            TextureAccessMode  texture_access_mode = TextureAccessMode::Static);

        static Texture* create(
            const std::string&  name,
            std::uint32_t       texture_width,
            std::uint32_t       texture_height,
            TextureFormat       texture_format,
            Flags<TextureUsage> usage_flags,
            TextureAccessMode   texture_access_mode = TextureAccessMode::Static);

        static Texture* get(const std::string& name);
        static void register_texture(const std::string& name, Texture* texture);
        static void destroy(Texture* texture);

        void upload(const void* data, std::size_t data_size, std::uint32_t frame_index = 0) const;
        bool save_to_file(const std::string& filepath, std::uint32_t frame_index = 0) const;

        void transition_layout(TextureLayout old_layout, TextureLayout new_layout, std::uint32_t frame_index = 0) const;

        void set_sampler_filter(SamplerFilter filter);
        void set_sampler_wrap(SamplerWrap wrap_u, SamplerWrap wrap_v, SamplerWrap wrap_w);

        PropertyGet<Texture, std::uint32_t> width{ &Texture::get_width, offsetof(Texture, width) };
        PropertyGet<Texture, std::uint32_t> height{ &Texture::get_height, offsetof(Texture, height) };
        PropertyGet<Texture, TextureFormat> format{ &Texture::get_format, offsetof(Texture, format) };

        PropertyGet<Texture, TextureAccessMode> access_mode
        {
            &Texture::get_access_mode,
            offsetof(Texture, access_mode)
        };

        [[nodiscard]] void* rhi_handle(std::uint32_t frame_index = 0) const;
        [[nodiscard]] void* rhi_sampler_handle() const { return rhi_sampler_; }

    private:
        [[nodiscard]] std::uint32_t     get_width() const { return width_; }
        [[nodiscard]] std::uint32_t     get_height() const { return height_; }
        [[nodiscard]] TextureFormat     get_format() const { return format_; }
        [[nodiscard]] TextureAccessMode get_access_mode() const { return access_mode_; }

        std::vector<void*> rhi_textures_;
        void*              rhi_sampler_;
        std::uint32_t      width_;
        std::uint32_t      height_;
        TextureFormat      format_;
        TextureAccessMode  access_mode_;
    };
}
