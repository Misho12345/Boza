module boza.gfx;

import boza.core;
import boza.gfx.texture_loader;
import boza.detail;
import boza.rhi;

namespace boza::gfx
{
    using detail::ImageIO;
    using detail::ImageData;

    // TODO: remove duplicate (same in texture.cpp)
    constexpr int channels_for_format(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R8:
            case TextureFormat::R16F:
            case TextureFormat::R32F: return 1;
            case TextureFormat::RG8:
            case TextureFormat::RG16F:
            case TextureFormat::RG32F: return 2;
            case TextureFormat::RGB8:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB32F: return 3;
            case TextureFormat::RGBA8:
            case TextureFormat::BGRA8:
            case TextureFormat::RGBA16F:
            case TextureFormat::RGBA32F: return 4;
            default: return 4;
        }
    }

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
            ResourceAccessMode::Static);

        if (error_texture_ && error_texture_->is_valid())
        {
            static constexpr std::array<std::uint8_t, 4> magenta{ 255, 0, 255, 255 };
            error_texture_->upload(magenta.data(), magenta.size());
            owned_textures_.insert(error_texture_);
            Log::trace("Created error texture (1x1 magenta)");
        }
        else
        {
            delete error_texture_;
            error_texture_ = nullptr;
            Log::error("Failed to create error texture");
        }

        initialized_ = true;
    }

    void TextureLoader::shutdown()
    {
        if (!initialized_) return;

        textures_.clear();

        for (const auto* texture : owned_textures_)
        {
            if (texture) delete texture;
        }
        owned_textures_.clear();

        error_texture_ = nullptr;
        initialized_   = false;
    }

    Texture* TextureLoader::get_or_load(
        const std::string&  filepath,
        const TextureFormat format,
        const SamplerFilter filter,
        const SamplerWrap   wrap)
    {
        const std::string key = make_texture_key(filepath, format);

        if (textures_.contains(key)) return textures_[key];

        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return error_texture_;
        }

        const auto      full_path        = detail::AssetPaths::texture(filepath);
        const int       desired_channels = channels_for_format(format);
        const ImageData image_data       = ImageIO::read(full_path.string(), desired_channels);

        if (!image_data.data)
        {
            Log::warn("Failed to load texture: {}, returning error texture", filepath);
            return error_texture_;
        }

        constexpr auto usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst;

        auto* texture = new Texture(
            image_data.width,
            image_data.height,
            format,
            usage_flags,
            ResourceAccessMode::Static,
            filter,
            wrap);

        if (!texture->is_valid())
        {
            Log::warn("Failed to create texture for: {}, returning error texture", filepath);
            delete texture;
            return error_texture_;
        }

        static_cast<rhi::Texture*>(texture->rhi_handle(0))->upload(image_data.data, image_data.size);

        textures_[key] = texture;
        if (!textures_.contains(filepath)) textures_[filepath] = texture;
        owned_textures_.insert(texture);

        Log::trace("Loaded texture: {} (format: {})", filepath, static_cast<int>(format));

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
                owned_textures_.erase(existing);
                delete existing;
            }
        }

        textures_[name] = texture;

        if (take_ownership)
        {
            owned_textures_.insert(texture);
        }

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
        return it != textures_.end() ? it->second : nullptr;
    }

    std::string TextureLoader::make_texture_key(const std::string& filepath, const TextureFormat format)
    {
        return filepath + "_" + std::to_string(static_cast<int>(format));
    }
}