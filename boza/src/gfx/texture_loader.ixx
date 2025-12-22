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

        Texture* load_or_get_texture(
            const std::string& filepath,
            TextureFormat format = TextureFormat::RGBA8,
            TextureAccessMode access_mode = TextureAccessMode::Static);

        void register_texture(const std::string& name, Texture* texture, bool take_ownership = true);
        void unregister_texture(Texture* texture);
        Texture* get_texture(const std::string& name) const;

        [[nodiscard]] Texture* error_texture() const { return error_texture_; }

    private:
        TextureLoader() = default;
        ~TextureLoader();

        Texture* load_texture(
            const std::string& filepath,
            TextureFormat format,
            TextureAccessMode access_mode) const;

        flat_map<std::string, Texture*> textures_;
        std::unordered_set<Texture*> owned_textures_;
        Texture* error_texture_{ nullptr };
        bool initialized_{ false };
    };
}
