module;

#include "api.hpp"

export module boza.gfx:texture;

import std;
import boza.common;
import :common;

namespace boza::gfx
{
    class TextureLoader;
}

export namespace boza
{
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

    constexpr BOZA_API Flags<TextureUsage> operator|(const TextureUsage left, const TextureUsage right) noexcept
    {
        return Flags(left) | Flags(right);
    }

    constexpr BOZA_API Flags<TextureUsage> operator&(const TextureUsage left, const TextureUsage right) noexcept
    {
        return Flags(left) & Flags(right);
    }

    constexpr BOZA_API Flags<TextureUsage> operator^(const TextureUsage left, const TextureUsage right) noexcept
    {
        return Flags(left) ^ Flags(right);
    }

    constexpr BOZA_API Flags<TextureUsage> operator~(const TextureUsage value) noexcept { return ~Flags(value); }

    enum class TextureType : std::uint8_t
    {
        Texture1D,
        Texture2D,
        Texture3D,
        TextureCube,
        Texture1DArray,
        Texture2DArray,
        TextureCubeArray,
    };

    struct TextureSettings final
    {
        TextureType         type{ TextureType::Texture2D };
        TextureFormat       format{ TextureFormat::RGBA8 };
        ResourceAccessMode  access_mode{ ResourceAccessMode::Static };
        std::uint32_t       width{ 1u };
        std::uint32_t       height{ 1u };
        std::uint32_t       depth{ 1u };
        Flags<TextureUsage> usage_flags{ TextureUsage::Sampled | TextureUsage::TransferDst };
    };

    class BOZA_API Texture final
    {
        [[nodiscard]] std::uint32_t get_width() const { return settings_.width; }
        [[nodiscard]] std::uint32_t get_height() const { return settings_.height; }
        [[nodiscard]] std::uint32_t get_depth() const { return settings_.depth; }
        [[nodiscard]] TextureType get_type() const { return settings_.type; }
        [[nodiscard]] TextureFormat get_format() const { return settings_.format; }

    public:
        ~Texture();

        Texture(const Texture&)            = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(Texture&&) noexcept;

        static Texture& copy(
            std::string_view src_name,
            std::string_view dst_name,
            ResourceAccessMode access_mode);

        static Texture& create(
            std::string_view name,
            const TextureSettings& settings);

        static Texture& get_or_load(std::string_view name);
        static Texture& get_or_load_cubemap(std::string_view name);

        static Texture& get(std::string_view name);
        static Texture* try_get(std::string_view name);

        static void destroy(std::string_view name);

        void upload(const void* data, std::size_t data_size, std::uint32_t frame_index = 0) const;
        void upload_layer(const void* data, std::size_t data_size, std::uint32_t layer, std::uint32_t frame_index = 0) const;

        [[nodiscard]] std::vector<std::uint8_t> read_back(std::uint32_t frame_index = 0) const;
        [[nodiscard]] bool save_to_file(const std::string& filepath, std::uint32_t frame_index = 0) const;

        void transition_layout(TextureLayout old_layout, TextureLayout new_layout, std::uint32_t frame_index = 0) const;

        [[msvc::no_unique_address]] Property<Texture, &Texture::get_width>  width{ this };
        [[msvc::no_unique_address]] Property<Texture, &Texture::get_height> height{ this };
        [[msvc::no_unique_address]] Property<Texture, &Texture::get_depth>  depth{ this };
        [[msvc::no_unique_address]] Property<Texture, &Texture::get_type>   type{ this };
        [[msvc::no_unique_address]] Property<Texture, &Texture::get_format> format{ this };

        [[nodiscard]] void* rhi_handle(std::uint32_t frame_index = 0) const;
        [[nodiscard]] bool  is_valid() const { return !rhi_textures_.empty(); }
        [[nodiscard]] std::string_view name() const { return name_; }

    private:
        Texture(std::string_view name, const TextureSettings& settings);
        void cleanup();

        [[nodiscard]] std::uint32_t resolve_texture_index(std::uint32_t frame_index) const;

        std::string name_;
        TextureSettings settings_;
        std::vector<void*> rhi_textures_{};

        friend class gfx::TextureLoader;
    };
}
