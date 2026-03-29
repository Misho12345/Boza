export module boza.gfx.texture_loader;

import std;
import boza.common;
import boza.core;
import boza.gfx;
import boza.gfx.common;
import boza.gfx.resource_registry;

export namespace boza::gfx
{
    class TextureLoader final
    {
    public:
        static TextureLoader& instance();

        TextureLoader(const TextureLoader&) = delete;
        TextureLoader& operator=(const TextureLoader&) = delete;
        TextureLoader(TextureLoader&&) = delete;
        TextureLoader& operator=(TextureLoader&&) = delete;

        void initialize();
        void shutdown();

        Texture& get_or_load(
            std::string_view name,
            TextureType type = TextureType::Texture2D);

        Texture& get_or_load_cubemap(std::string_view name);

        Texture& copy(
            std::string_view src_name,
            std::string_view dst_name,
            ResourceAccessMode access_mode);

        [[nodiscard]] Texture* try_get_texture(std::string_view name);
        [[nodiscard]] bool exists(const Texture* ptr) const;

        [[nodiscard]] Texture& error_texture(TextureType type);

        [[nodiscard]] bool initialized() const { return initialized_; }

    private:
        TextureLoader() = default;
        ~TextureLoader();

        Texture& create(
            std::string_view name,
            const TextureSettings& settings);

        void destroy(std::string_view name);

        static std::string error_texture_key(TextureType type);

        static constexpr std::size_t error_texture_count_ = static_cast<std::size_t>(TextureType::TextureCubeArray) + 1;

        ResourceRegistry<Texture> textures_;
        std::array<Texture*, error_texture_count_> error_textures_{};
        bool initialized_{ false };

        friend class Texture;
    };
}
