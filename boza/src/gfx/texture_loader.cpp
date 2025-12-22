module boza.gfx;

import boza.core;
import boza.gfx.texture_loader;
import boza.detail;

namespace boza::gfx
{
    TextureLoader& TextureLoader::instance()
    {
        static TextureLoader instance;
        return instance;
    }

    TextureLoader::~TextureLoader() { shutdown(); }

    void TextureLoader::initialize()
    {
        if (initialized_) return;

        error_texture_ = new Texture(
            1, 1,
            TextureFormat::RGBA8,
            TextureUsage::Sampled | TextureUsage::TransferDst,
            TextureAccessMode::Static);

        if (error_texture_)
        {
            static constexpr std::array<std::uint8_t, 4> magenta{ 255, 0, 255, 255 };
            error_texture_->upload(magenta.data(), magenta.size());
            owned_textures_.insert(error_texture_);
            Log::trace("Created error texture (1x1 magenta)");
        }

        initialized_ = true;
    }

    void TextureLoader::shutdown()
    {
        if (!initialized_) return;

        textures_.clear();

        for (const auto* texture : owned_textures_) { if (texture) delete texture; }
        owned_textures_.clear();

        error_texture_ = nullptr;
        initialized_   = false;
    }

    Texture* TextureLoader::load_texture(
        const std::string&      filepath,
        const TextureFormat     format,
        const TextureAccessMode access_mode) const
    {
        const auto tex_path = detail::AssetPaths::texture(filepath);
        auto*      loaded   = Texture::load_from_file(tex_path.string(), format, access_mode);

        if (loaded)
        {
            Log::trace("Loaded texture: {} (format: {})", filepath, static_cast<int>(format));
            return loaded;
        }

        Log::warn("Failed to load texture: {}, returning error texture", filepath);
        return error_texture_;
    }

    Texture* TextureLoader::load_or_get_texture(
        const std::string&      filepath,
        const TextureFormat     format,
        const TextureAccessMode access_mode)
    {
        const std::string key = filepath + "_" + std::to_string(static_cast<int>(format));

        if (textures_.contains(key)) return textures_[key];

        auto* texture = load_texture(filepath, format, access_mode);
        if (texture && texture != error_texture_)
        {
            textures_[key]      = texture;
            textures_[filepath] = texture;
            owned_textures_.insert(texture);
        }

        return texture;
    }

    void TextureLoader::register_texture(const std::string& name, Texture* texture, const bool take_ownership)
    {
        if (!texture)
        {
            Log::warn("Cannot register null texture '{}'", name);
            return;
        }

        if (textures_.contains(name))
        {
            auto* existing = textures_[name];
            if (existing != texture && owned_textures_.contains(existing))
            {
                Log::warn("Texture '{}' already registered, replacing and deleting old texture", name);
                delete existing;
                owned_textures_.erase(existing);
            }
        }

        textures_[name] = texture;

        if (take_ownership) { owned_textures_.insert(texture); }

        Log::trace("Registered texture: {} (owned: {})", name, take_ownership);
    }

    void TextureLoader::unregister_texture(Texture* texture)
    {
        if (!texture) return;

        for (auto it = textures_.begin(); it != textures_.end();)
        {
            if (it->second == texture)
            {
                Log::trace("Unregistered texture: {}", it->first);
                it = textures_.erase(it);
            }
            else ++it;
        }

        owned_textures_.erase(texture);
    }

    Texture* TextureLoader::get_texture(const std::string& name) const
    {
        const auto it = textures_.find(name);
        if (it != textures_.end()) return it->second;
        return nullptr;
    }
}
