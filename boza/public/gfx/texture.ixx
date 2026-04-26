module;

#include "api.hpp"

export module boza.gfx:texture;

import std;
import boza.common;
import boza.gfx.common;
import :buffer;

namespace boza::gfx
{
    class TextureLoader;
}

namespace boza
{
    class Texture;

    struct TextureAccess
    {
        static void* handle(const Texture& texture);
        static void* handle(const Texture& texture, std::uint32_t frame_index);
        static TextureLayout layout(const Texture& texture);
        static void set_layout(Texture& texture, TextureLayout layout);
    };
}

export namespace boza
{
    class Material;

    struct TextureSettings final
    {
        TextureType type{ TextureType::Texture2D };
        TextureFormat format{ TextureFormat::RGBA8 };
        ResourceAccessMode access_mode{ ResourceAccessMode::Static };
        std::uint32_t width{ 1u };
        std::uint32_t height{ 1u };
        std::uint32_t depth{ 1u };
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
            std::string_view   src_name,
            std::string_view   dst_name,
            ResourceAccessMode access_mode);

        static Texture& create(
            std::string_view name,
            const TextureSettings& settings);

        static Texture& get_or_load(std::string_view name);
        static Texture& get_or_load_cubemap(std::string_view name);

        static Texture& get(std::string_view name);
        static Texture* try_get(std::string_view name);
        static bool exists(const Texture* ptr);

        static void destroy(std::string_view name);

        void upload(const void* data, std::size_t data_size) const;
        void upload_layer(const void* data, std::size_t data_size, std::uint32_t layer) const;
        void upload_from(const Buffer& staging_buffer, std::uint32_t layer = 0, std::size_t size = 0) const;

        [[nodiscard]] Buffer stage(std::size_t size = 0, std::uint32_t layer = 0) const;

        [[nodiscard]] std::vector<std::uint8_t> read_back() const;
        [[nodiscard]] bool save_to_file(const std::string& filepath) const;

        void transition_layout(TextureLayout old_layout, TextureLayout new_layout) const;

        [[msvc::no_unique_address]] Property<Texture, &Texture::get_width>  width{ this };
        [[msvc::no_unique_address]] Property<Texture, &Texture::get_height> height{ this };
        [[msvc::no_unique_address]] Property<Texture, &Texture::get_depth>  depth{ this };
        [[msvc::no_unique_address]] Property<Texture, &Texture::get_type>   type{ this };
        [[msvc::no_unique_address]] Property<Texture, &Texture::get_format> format{ this };

        [[nodiscard]] bool  is_valid() const { return !rhi_textures_.empty(); }
        [[nodiscard]] std::string_view name() const { return name_; }

    private:
        using RhiTextureHandle = std::unique_ptr<void, void(*)(void*)>;

        [[nodiscard]] void* rhi_handle() const;
        [[nodiscard]] void* rhi_handle(std::uint32_t frame_index) const;

        static void destroy_rhi_texture(void* handle);

        Texture(std::string_view name, const TextureSettings& settings);
        void cleanup();

        [[nodiscard]] void* get_validated_texture() const;

        std::string name_;
        TextureSettings settings_;
        std::vector<RhiTextureHandle> rhi_textures_{};
        std::vector<TextureLayout> layouts_{};

        friend struct TextureAccess;
        friend class Material;
        friend class ComputeDispatcher;
        friend class gfx::TextureLoader;
    };
}
