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

        Texture* get_or_load(
            const std::string& name,
            TextureType type = TextureType::Texture2D);

        Texture* get_or_load_cubemap(const std::string& name);

        Texture* copy(
            const std::string& src_name,
            const std::string& dst_name,
            ResourceAccessMode access_mode);

        void register_texture(const std::string& name, Texture* texture, bool take_ownership = true);
        void unregister_texture(Texture* texture);

        [[nodiscard]] Texture* get_texture(const std::string& name) const;

        [[nodiscard]] Texture* error_texture() const { return error_texture_; }

        [[nodiscard]] bool initialized() const { return initialized_; }

    private:
        TextureLoader() = default;
        ~TextureLoader();

        static std::string make_texture_key(const std::string& filepath, TextureType type);

        flat_map<std::string, Texture*> textures_;
        std::unordered_set<Texture*> owned_textures_;
        Texture* error_texture_{ nullptr };
        bool initialized_{ false };
    };
}