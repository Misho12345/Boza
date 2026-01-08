module boza.gfx.texture_loader;

import boza.core;
import boza.gfx;
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
            case 3: return TextureFormat::RGB8;
            case 4: return TextureFormat::RGBA8;
            default: std::unreachable();
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

        auto [it, inserted] = textures_.try_emplace(
            "boza_error_texture",
            Texture{
                "boza_error_texture",
                TextureSettings{
                    .type = TextureType::Texture2D,
                    .format = TextureFormat::RGBA8,
                    .access_mode = ResourceAccessMode::Static,
                    .width = 1,
                    .height = 1,
                    .depth = 1,
                    .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
                }
            });

        error_texture_ = &it->second;

        if (error_texture_ && error_texture_->is_valid())
        {
            static constexpr std::array<std::uint8_t, 4> magenta{ 255, 0, 255, 255 };
            error_texture_->upload(magenta.data(), magenta.size());
        }
        else
        {
            Log::error("Failed to create error texture");
        }

        initialized_ = true;
    }

    void TextureLoader::shutdown()
    {
        if (!initialized_) return;

        textures_.clear();

        error_texture_ = nullptr;
        initialized_   = false;
    }

    Texture& TextureLoader::get_or_load(
        const std::string_view name,
        const TextureType type)
    {
        const std::string name_str{ name };
        const std::string key = make_texture_key(name_str, type);

        if (auto it = textures_.find(key); it != textures_.end()) return it->second;
        if (auto it = textures_.find(name_str); it != textures_.end()) return it->second;

        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return *error_texture_;
        }

        const auto      full_path  = detail::AssetPaths::texture(name_str);
        const ImageData image_data = ImageIO::read(full_path.string());

        if (!image_data.data)
        {
            Log::warn("Failed to load texture: {}, returning error texture", name_str);
            return *error_texture_;
        }

        auto [it, inserted] = textures_.try_emplace(
            name_str,
            Texture{
                name_str,
                TextureSettings{
                    .type = type,
                    .format = format_from_channels(image_data.channels),
                    .access_mode = ResourceAccessMode::Static,
                    .width = image_data.width,
                    .height = image_data.height,
                    .depth = 1,
                    .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
                }
            });

        Texture& texture = it->second;

        if (!texture.is_valid())
        {
            Log::warn("Failed to create texture for: {}, returning error texture", name_str);
            textures_.erase(it);
            return *error_texture_;
        }

        texture.upload(image_data.data, image_data.size, 0);

        return texture;
    }

    Texture& TextureLoader::get_or_load_cubemap(const std::string_view name)
    {
        const std::string name_str{ name };
        const std::string key = make_texture_key(name_str, TextureType::TextureCube);

        if (auto it = textures_.find(key); it != textures_.end())
            return it->second;

        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return *error_texture_;
        }

        const auto      full_path  = detail::AssetPaths::texture(name_str);
        const ImageData image_data = ImageIO::read(full_path.string());

        if (!image_data.data)
        {
            Log::warn("Failed to load cubemap texture: {}, returning error texture", name_str);
            return *error_texture_;
        }

        const std::uint32_t face_width  = image_data.width / 4;
        const std::uint32_t face_height = image_data.height / 3;

        if (face_width == 0 || face_height == 0 ||
            image_data.width % 4 != 0 || image_data.height % 3 != 0)
        {
            Log::warn("Cubemap texture {} has invalid dimensions ({}x{}), expected 4:3 ratio for cross layout",
                      name_str, image_data.width, image_data.height);
            return *error_texture_;
        }

        auto [it, inserted] = textures_.try_emplace(
            key,
            Texture{
                key,
                TextureSettings{
                    .type = TextureType::TextureCube,
                    .format = format_from_channels(image_data.channels),
                    .access_mode = ResourceAccessMode::Static,
                    .width = face_width,
                    .height = face_height,
                    .depth = 1,
                    .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
                }
            });

        Texture& texture = it->second;

        if (!texture.is_valid())
        {
            Log::warn("Failed to create cubemap texture for: {}, returning error texture", name_str);
            textures_.erase(it);
            return *error_texture_;
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

        extract_face(2, 1);
        texture.upload_layer(face_data.data(), face_size, 0);

        extract_face(0, 1);
        texture.upload_layer(face_data.data(), face_size, 1);

        extract_face(1, 0);
        texture.upload_layer(face_data.data(), face_size, 2);

        extract_face(1, 2);
        texture.upload_layer(face_data.data(), face_size, 3);

        extract_face(1, 1);
        texture.upload_layer(face_data.data(), face_size, 4);

        extract_face(3, 1);
        texture.upload_layer(face_data.data(), face_size, 5);

        return texture;
    }

    Texture& TextureLoader::copy(
        const std::string_view src_name,
        const std::string_view dst_name,
        const ResourceAccessMode access_mode)
    {
        const std::string src_str{ src_name };
        const std::string dst_str{ dst_name };

        auto it = textures_.find(src_str);
        if (it == textures_.end())
        {
            Log::error("Cannot copy texture '{}', not found", src_str);
            std::abort();
        }

        const Texture& src_texture = it->second;

        const auto data = src_texture.read_back();

        auto [dst_it, inserted] = textures_.try_emplace(
            dst_str,
            Texture{
                dst_str,
                TextureSettings{
                    .type = src_texture.type,
                    .format = src_texture.format,
                    .access_mode = access_mode,
                    .width = src_texture.width,
                    .height = src_texture.height,
                    .depth = src_texture.depth,
                    .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
            }});

        Texture& dst_texture = dst_it->second;

        if (!dst_texture.is_valid())
        {
            Log::error("Failed to create texture copy for: {}", dst_str);
            textures_.erase(dst_it);
            std::abort();
        }

        dst_texture.upload(data.data(), data.size());

        return dst_texture;
    }

    Texture& TextureLoader::create(
        const std::string_view name,
        const TextureSettings& settings)
    {
        const std::string name_str{ name };

        if (textures_.contains(name_str))
        {
            Log::warn("Texture '{}' already exists, returning existing texture", name_str);
            return textures_.at(name_str);
        }

        auto [it, inserted] = textures_.try_emplace(name_str, std::move(Texture{ name, settings }));

        if (!inserted || !it->second.is_valid())
        {
            Log::error("Failed to create texture: {}", name_str);
            if (inserted) textures_.erase(it);
            return *error_texture_;
        }

        // Log::trace("Created texture: {}", name_str);
        return it->second;
    }

    void TextureLoader::destroy(const std::string_view name)
    {
        const std::string name_str{ name };

        if (name_str == "boza_error_texture")
        {
            Log::warn("Cannot destroy error texture");
            return;
        }

        const auto it = textures_.find(name_str);
        if (it != textures_.end())
        {
            // Log::trace("Destroyed texture: {}", name_str);
            textures_.erase(it);
        }
        else Log::warn("Attempted to destroy non-existent texture: {}", name_str);
    }

    Texture* TextureLoader::try_get_texture(const std::string_view name)
    {
        const std::string name_str{ name };
        const auto it = textures_.find(name_str);
        return it != textures_.end() ? &it->second : nullptr;
    }

    std::string TextureLoader::make_texture_key(const std::string_view filepath, const TextureType type)
    {
        return std::string{ filepath } + "_" + std::to_string(static_cast<int>(type));
    }
}
