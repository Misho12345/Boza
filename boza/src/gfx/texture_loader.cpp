module boza.gfx.texture_loader;

import boza.core;
import boza.gfx;

import boza.detail;

import boza.rhi;
import boza.rhi.render_context;

namespace boza::gfx
{
    using detail::ImageIO;
    using detail::ImageData;

    namespace
    {
        constexpr TextureSettings error_texture_settings(const TextureType type)
        {
            return TextureSettings{
                .type = type,
                .format = TextureFormat::RGBA8,
                .access_mode = ResourceAccessMode::Static,
                .width = 1,
                .height = 1,
                .depth = 1,
                .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
            };
        }

        constexpr std::uint32_t error_texture_layer_count(const TextureType type)
        {
            return type == TextureType::TextureCube || type == TextureType::TextureCubeArray ? 6 : 1;
        }
    }

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

        static constexpr std::array<std::uint8_t, 4> magenta{ 255, 0, 255, 255 };
        bool created_any_error_texture = false;

        error_textures_.fill(nullptr);

        for (std::size_t i = 0; i < error_texture_count_; ++i)
        {
            const auto        type = static_cast<TextureType>(i);
            const std::string key = error_texture_key(type);

            auto [texture, inserted] = textures_.try_emplace(
                key,
                Texture{ key, error_texture_settings(type) });

            error_textures_[i] = texture;

            if (!texture || !texture->is_valid())
            {
                Log::error("Failed to create error texture '{}' for type {}", key, static_cast<int>(type));
                if (inserted) textures_.erase(key);
                error_textures_[i] = nullptr;
                continue;
            }

            created_any_error_texture = true;

            for (std::uint32_t layer = 0; layer < error_texture_layer_count(type); ++layer)
            {
                texture->upload_layer(magenta.data(), magenta.size(), layer);
            }
        }

        initialized_ = created_any_error_texture;
    }

    void TextureLoader::shutdown()
    {
        if (!initialized_) return;

        textures_.clear();

        error_textures_.fill(nullptr);
        initialized_   = false;
    }

    Texture& TextureLoader::error_texture(const TextureType type)
    {
        Texture*& cached_error_texture = error_textures_[static_cast<int>(type)];
        if (cached_error_texture) return *cached_error_texture;

        const std::string error_key = error_texture_key(type);
        cached_error_texture = textures_.find_ptr(error_key);
        if (cached_error_texture) return *cached_error_texture;

        if (!initialized_ && rhi::RenderContext::initialized())
        {
            initialize();
            cached_error_texture = textures_.find_ptr(error_key);
            if (cached_error_texture) return *cached_error_texture;
        }

        Log::critical("TextureLoader error texture '{}' is unavailable", error_key);
        assert(cached_error_texture != nullptr, "TextureLoader error texture is not available");
        std::terminate();
    }

    Texture& TextureLoader::get_or_load(
        const std::string_view name,
        const TextureType type)
    {
        const std::string name_str{ name };

        if (auto* texture = textures_.find_ptr(name_str))
        {
            if (texture->type != type)
            {
                Log::error(
                    "Texture '{}' already exists with type {}, requested type {}",
                    name_str,
                    static_cast<int>(texture->settings_.type),
                    static_cast<int>(type));
                return error_texture(type);
            }

            return *texture;
        }

        if (!rhi::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return error_texture(type);
        }

        const auto      full_path  = detail::AssetPaths::texture(name_str);
        const ImageData image_data = ImageIO::read(full_path.string());

        if (!image_data.data)
        {
            Log::warn("Failed to load texture: {}, returning error texture", name_str);
            return error_texture(type);
        }

        auto [texture, inserted] = textures_.try_emplace(
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

        if (!texture || !texture->is_valid())
        {
            Log::warn("Failed to create texture for: {}, returning error texture", name_str);
            if (inserted) textures_.erase(name_str);
            return error_texture(type);
        }

        texture->upload(image_data.data, image_data.size);

        return *texture;
    }

    Texture& TextureLoader::get_or_load_cubemap(const std::string_view name)
    {
        const std::string name_str{ name };

        if (auto* texture = textures_.find_ptr(name_str))
        {
            if (texture->type != TextureType::TextureCube)
            {
                Log::error(
                    "Texture '{}' already exists with type {}, requested type {}",
                    name_str,
                    static_cast<int>(texture->settings_.type),
                    static_cast<int>(TextureType::TextureCube));
                return error_texture(TextureType::TextureCube);
            }

            return *texture;
        }

        if (!rhi::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return error_texture(TextureType::TextureCube);
        }

        const auto      full_path  = detail::AssetPaths::texture(name_str);
        const ImageData image_data = ImageIO::read(full_path.string());

        if (!image_data.data)
        {
            Log::warn("Failed to load cubemap texture: {}, returning error texture", name_str);
            return error_texture(TextureType::TextureCube);
        }

        const std::uint32_t face_width  = image_data.width / 4;
        const std::uint32_t face_height = image_data.height / 3;

        if (face_width == 0 || face_height == 0 ||
            image_data.width % 4 != 0 || image_data.height % 3 != 0)
        {
            Log::warn("Cubemap texture {} has invalid dimensions ({}x{}), expected 4:3 ratio for cross layout",
                      name_str, image_data.width, image_data.height);
            return error_texture(TextureType::TextureCube);
        }

        auto [texture, inserted] = textures_.try_emplace(
            name_str,
            Texture{
                name_str,
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

        if (!texture || !texture->is_valid())
        {
            Log::warn("Failed to create cubemap texture for: {}, returning error texture", name_str);
            if (inserted) textures_.erase(name_str);
            return error_texture(TextureType::TextureCube);
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
        texture->upload_layer(face_data.data(), face_size, 0);

        extract_face(0, 1);
        texture->upload_layer(face_data.data(), face_size, 1);

        extract_face(1, 0);
        texture->upload_layer(face_data.data(), face_size, 2);

        extract_face(1, 2);
        texture->upload_layer(face_data.data(), face_size, 3);

        extract_face(1, 1);
        texture->upload_layer(face_data.data(), face_size, 4);

        extract_face(3, 1);
        texture->upload_layer(face_data.data(), face_size, 5);

        return *texture;
    }

    Texture& TextureLoader::copy(
        const std::string_view src_name,
        const std::string_view dst_name,
        const ResourceAccessMode access_mode)
    {
        const std::string src_str{ src_name };
        const std::string dst_str{ dst_name };

        auto fallback_type = TextureType::Texture2D;
        if (const auto* existing_dst = textures_.find_ptr(dst_str)) fallback_type = existing_dst->type;

        const Texture* src_texture = try_get_texture(src_str);
        if (!src_texture)
        {
            Log::error("Cannot copy texture '{}', not found", src_str);
            return error_texture(fallback_type);
        }

        if (auto* existing_dst = textures_.find_ptr(dst_str); existing_dst && existing_dst->type != src_texture->type)
        {
            Log::error(
                "Texture '{}' already exists with type {}, requested type {}",
                dst_str,
                static_cast<int>(existing_dst->settings_.type),
                static_cast<int>(src_texture->settings_.type));
            return error_texture(src_texture->settings_.type);
        }

        const auto data = src_texture->read_back();

        auto [dst_texture, inserted] = textures_.try_emplace(
            dst_str,
            Texture{
                dst_str,
                TextureSettings{
                    .type = src_texture->type,
                    .format = src_texture->format,
                    .access_mode = access_mode,
                    .width = src_texture->width,
                    .height = src_texture->height,
                    .depth = src_texture->depth,
                    .usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst
            }});

        if (!dst_texture || !dst_texture->is_valid())
        {
            Log::error("Failed to create texture copy for: {}", dst_str);
            if (inserted) textures_.erase(dst_str);
            return error_texture(src_texture->type);
        }

        dst_texture->upload(data.data(), data.size());

        return *dst_texture;
    }

    Texture& TextureLoader::create(
        const std::string_view name,
        const TextureSettings& settings)
    {
        const std::string name_str{ name };

        if (auto* existing = textures_.find_ptr(name_str))
        {
            if (existing->type != settings.type)
            {
                Log::error(
                    "Texture '{}' already exists with type {}, requested type {}",
                    name_str,
                    static_cast<int>(existing->settings_.type),
                    static_cast<int>(settings.type));
                return error_texture(settings.type);
            }

            Log::warn("Texture '{}' already exists, returning existing texture", name_str);
            return *existing;
        }

        auto [texture, inserted] = textures_.try_emplace(name_str, std::move(Texture{ name, settings }));

        if (!texture || !texture->is_valid())
        {
            Log::error("Failed to create texture: {}", name_str);
            if (inserted) textures_.erase(name_str);
            return error_texture(settings.type);
        }

        // Log::trace("Created texture: {}", name_str);
        return *texture;
    }

    void TextureLoader::destroy(const std::string_view name)
    {
        const std::string name_str{ name };

        for (std::size_t i = 0; i < error_texture_count_; ++i)
        {
            if (name_str == error_texture_key(static_cast<TextureType>(i)))
            {
                Log::warn("Cannot destroy error texture");
                return;
            }
        }

        if (!textures_.erase(name_str))
        {
            Log::warn("Attempted to destroy non-existent texture: {}", name_str);
        }
    }

    Texture* TextureLoader::try_get_texture(const std::string_view name)
    {
        return textures_.find_ptr(name);
    }

    bool TextureLoader::exists(const Texture* ptr) const { return textures_.exists(ptr); }

    std::string TextureLoader::error_texture_key(const TextureType type)
    {
        return "boza_error_texture_" + std::to_string(static_cast<int>(type));
    }
}
