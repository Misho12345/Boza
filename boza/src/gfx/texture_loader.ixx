export module boza.gfx.texture_loader;

import std;
import boza.common;
import boza.core;
import boza.gfx;

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

        [[nodiscard]] Texture& error_texture() { return *error_texture_; }

        [[nodiscard]] bool initialized() const { return initialized_; }

    private:
        TextureLoader() = default;
        ~TextureLoader();

        Texture& create(
            std::string_view name,
            const TextureSettings& settings);

        void destroy(std::string_view name);

        static std::string make_texture_key(std::string_view filepath, TextureType type);

        mt::node_map<std::string, Texture> textures_;
        Texture* error_texture_{ nullptr };
        bool initialized_{ false };

        friend class Texture;
    };
}