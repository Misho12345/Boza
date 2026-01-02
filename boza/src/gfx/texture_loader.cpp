module boza.gfx;

import boza.core;
import boza.gfx.texture_loader;
import boza.detail;
import boza.rhi;

namespace boza::gfx
{
    using detail::ImageIO;
    using detail::ImageData;

    constexpr TextureFormat format_from_channels(const int channels)
    {
        switch (channels)
        {
            case 1: return TextureFormat::R8;
            case 2: return TextureFormat::RG8;
            default: return TextureFormat::RGBA8;
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

        error_texture_ = Texture::create(
            "boza_error_texture",
            {
                .type = TextureType::Texture2D,
                .format = TextureFormat::RGBA8,
                .access_mode = ResourceAccessMode::Static,
                .width = 1,
                .height = 1,
                .depth = 1,
                .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
            });

        if (error_texture_ && error_texture_->is_valid())
        {
            static constexpr std::array<std::uint8_t, 4> magenta{ 255, 0, 255, 255 };
            error_texture_->upload(magenta.data(), magenta.size());
            owned_textures_.insert(error_texture_);
            // Log::trace("Created error texture (1x1 magenta)");
        }
        else
        {
            Texture::destroy(error_texture_);
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
        const std::string&  name,
        TextureType type)
    {
        const std::string key = make_texture_key(name, type);

        if (textures_.contains(key)) return textures_[key];

        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return error_texture_;
        }

        const auto      full_path  = detail::AssetPaths::texture(name);
        const ImageData image_data = ImageIO::read(full_path.string());

        if (!image_data.data)
        {
            Log::warn("Failed to load texture: {}, returning error texture", name);
            return error_texture_;
        }

        const TextureFormat format = format_from_channels(image_data.channels);

        auto* texture = new Texture({
            .type = type,
            .format = format,
            .access_mode = ResourceAccessMode::Static,
            .width = image_data.width,
            .height = image_data.height,
            .depth = 1,
            .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
        });

        if (!texture->is_valid())
        {
            Log::warn("Failed to create texture for: {}, returning error texture", name);
            delete texture;
            return error_texture_;
        }

        texture->upload(image_data.data, image_data.size, 0);

        textures_[key] = texture;
        textures_[name] = texture;
        owned_textures_.insert(texture);

        return texture;
    }

    Texture* TextureLoader::get_or_load_cubemap(const std::string& name)
    {
        const std::string key = make_texture_key(name, TextureType::TextureCube);

        if (textures_.contains(key)) return textures_[key];

        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return error_texture_;
        }

        const auto      full_path  = detail::AssetPaths::texture(name);
        const ImageData image_data = ImageIO::read(full_path.string());

        if (!image_data.data)
        {
            Log::warn("Failed to load cubemap texture: {}, returning error texture", name);
            return error_texture_;
        }

        const TextureFormat format = format_from_channels(image_data.channels);

        const std::uint32_t face_width  = image_data.width / 4;
        const std::uint32_t face_height = image_data.height / 3;

        if (face_width == 0 || face_height == 0 ||
            image_data.width % 4 != 0 || image_data.height % 3 != 0)
        {
            Log::warn("Cubemap texture {} has invalid dimensions ({}x{}), expected 4:3 ratio for cross layout",
                      name, image_data.width, image_data.height);
            return error_texture_;
        }

        auto* texture = new Texture({
            .type = TextureType::TextureCube,
            .format = format,
            .access_mode = ResourceAccessMode::Static,
            .width = face_width,
            .height = face_height,
            .depth = 1,
            .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
        });

        if (!texture->is_valid())
        {
            Log::warn("Failed to create cubemap texture for: {}, returning error texture", name);
            delete texture;
            return error_texture_;
        }

        const std::size_t face_size       = face_width * face_height * image_data.channels;
        const std::size_t row_pitch       = image_data.width * image_data.channels;
        const std::size_t face_row_pitch  = face_width * image_data.channels;

        std::vector<std::uint8_t> face_data(face_size);

        auto extract_face = [&](const std::uint32_t grid_x, const std::uint32_t grid_y) {
            for (std::uint32_t y = 0; y < face_height; ++y)
            {
                const std::size_t src_offset =
                    (grid_y * face_height + y) * row_pitch +
                    grid_x * face_row_pitch;
                const std::size_t dst_offset = y * face_row_pitch;
                std::memcpy(face_data.data() + dst_offset, image_data.data + src_offset, face_row_pitch);
            }
        };

        // Cubemap face order in Vulkan: +X, -X, +Y, -Y, +Z, -Z
        // Cross layout (horizontal):
        //     [  ] [+Y] [  ] [  ]   <- row 0
        //     [-X] [+Z] [+X] [-Z]   <- row 1
        //     [  ] [-Y] [  ] [  ]   <- row 2
        // Grid positions: (col, row)

        // Face 0: +X (right)  -> grid(2, 1)
        extract_face(2, 1);
        texture->upload_layer(face_data.data(), face_size, 0);

        // Face 1: -X (left)   -> grid(0, 1)
        extract_face(0, 1);
        texture->upload_layer(face_data.data(), face_size, 1);

        // Face 2: +Y (top)    -> grid(1, 0)
        extract_face(1, 0);
        texture->upload_layer(face_data.data(), face_size, 2);

        // Face 3: -Y (bottom) -> grid(1, 2)
        extract_face(1, 2);
        texture->upload_layer(face_data.data(), face_size, 3);

        // Face 4: +Z (front)  -> grid(1, 1)
        extract_face(1, 1);
        texture->upload_layer(face_data.data(), face_size, 4);

        // Face 5: -Z (back)   -> grid(3, 1)
        extract_face(3, 1);
        texture->upload_layer(face_data.data(), face_size, 5);

        textures_[key] = texture;
        textures_[name + "_cube"] = texture;
        owned_textures_.insert(texture);

        // Log::trace("Loaded cubemap texture: {} (format: {})", name, static_cast<int>(format));

        return texture;
    }

    Texture* TextureLoader::copy(
        const std::string&       src_name,
        const std::string&       dst_name,
        const ResourceAccessMode access_mode)
    {
        if (!textures_.contains(src_name))
        {
            Log::warn("Cannot copy texture '{}', not found", src_name);
            return nullptr;
        }

        const Texture& src_texture = *textures_[src_name];

        Texture* dst_texture = Texture::create(dst_name, {
            .type = src_texture.type,
            .format = src_texture.format,
            .access_mode = access_mode,
            .width = src_texture.width,
            .height = src_texture.height,
            .depth = src_texture.depth,
            .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
        });

        if (!dst_texture->is_valid())
        {
            Log::warn("Failed to create texture for: {}, returning error texture", dst_name);
            Texture::destroy(dst_texture);
            return nullptr;
        }

        const auto data = src_texture.read_back();
        dst_texture->upload(data.data(), data.size());

        const std::string key = make_texture_key(dst_name, dst_texture->type);
        textures_[key] = dst_texture;
        owned_textures_.insert(dst_texture);

        return dst_texture;
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

        if (take_ownership) owned_textures_.insert(texture);
        // Log::trace("Registered texture: {} (owned: {})", name, take_ownership);
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

    std::string TextureLoader::make_texture_key(const std::string& filepath, const TextureType type)
    {
        return filepath + "_" + std::to_string(static_cast<int>(type));
    }
}