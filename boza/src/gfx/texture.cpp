module boza.gfx;

import :texture;

import boza.common;
import boza.core;

import boza.detail;

import boza.rhi;
import boza.rhi.render_context;

import boza.gfx.texture_loader;

namespace boza
{
    constexpr std::uint8_t channels_for_format(const TextureFormat format)
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

    Texture::Texture(const std::string_view name, const TextureSettings& settings)
        : name_{ name },
          settings_{ settings }
    {
        if (!rhi::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return;
        }

        const std::uint32_t texture_count = settings_.access_mode == ResourceAccessMode::Dynamic
                                                ? rhi::RenderContext::frames_in_flight()
                                                : 1;

        rhi_textures_.reserve(texture_count);

        const bool is_cube =
                settings_.type == TextureType::TextureCube ||
                settings_.type == TextureType::TextureCubeArray ||
                settings_.type == TextureType::Texture3D;

        for (std::uint32_t i = 0; i < texture_count; ++i)
        {
            void* rhi_texture = create_texture(
                rhi::RenderContext::api(), {
                    .device = (rhi::RenderContext::device()),
                    .type = settings_.type,
                    .format = settings_.format,
                    .usage = settings_.usage_flags,
                    .width = settings_.width,
                    .height = settings_.height,
                    .depth = settings_.depth,
                    .array_layers = is_cube ? 1 : settings_.depth,
                });

            if (!rhi_texture)
            {
                Log::error("Failed to create texture {} of {} ({}x{}x{})",
                           i, texture_count,
                           settings_.width,
                           settings_.height,
                           settings_.depth);
                cleanup();
                return;
            }

            rhi_textures_.push_back(rhi_texture);
        }
    }

    Texture::~Texture() { cleanup(); }

    void Texture::cleanup()
    {
        for (auto* rhi_texture : rhi_textures_)
        {
            if (rhi_texture)
            {
                auto* texture = static_cast<rhi::Texture*>(rhi_texture);
                texture->destroy();
                delete texture;
            }
        }

        rhi_textures_.clear();
    }

    Texture::Texture(Texture&& other) noexcept
        : name_{ std::move(other.name_) },
          settings_{ std::exchange(other.settings_, {}) },
          rhi_textures_{ std::move(other.rhi_textures_) } {}

    Texture& Texture::operator=(Texture&& other) noexcept
    {
        if (this == &other) return *this;

        cleanup();

        name_ = std::move(other.name_);
        rhi_textures_ = std::move(other.rhi_textures_);
        settings_     = std::exchange(other.settings_, {});

        return *this;
    }

    Texture& Texture::get_or_load(const std::string_view name)
    {
        return gfx::TextureLoader::instance().get_or_load(name);
    }

    Texture& Texture::get_or_load_cubemap(std::string_view name)
    {
        return gfx::TextureLoader::instance().get_or_load_cubemap(name);
    }

    Texture& Texture::copy(
        const std::string_view src_name,
        const std::string_view dst_name,
        const ResourceAccessMode access_mode)
    {
        return gfx::TextureLoader::instance().copy(src_name, dst_name, access_mode);
    }

    Texture& Texture::create(
        const std::string_view name,
        const TextureSettings& settings)
    {
        return gfx::TextureLoader::instance().create(name, settings);
    }

    Texture& Texture::get(const std::string_view name)
    {
        auto* texture = try_get(name);
        assert(texture != nullptr, "Texture not found");
        return *texture;
    }

    Texture* Texture::try_get(const std::string_view name)
    {
        return gfx::TextureLoader::instance().try_get_texture(name);
    }

    bool Texture::exists(const Texture* ptr)
    {
        return gfx::TextureLoader::instance().exists(ptr);
    }

    void Texture::destroy(std::string_view name)
    {
        gfx::TextureLoader::instance().destroy(name);
    }

    void Texture::upload(const void* data, const std::size_t data_size) const
    {
        upload_layer(data, data_size, 0);
    }

    void Texture::upload_layer(const void* data, const std::size_t data_size, const std::uint32_t layer) const
    {
        if (auto* texture = static_cast<rhi::Texture*>(get_validated_texture()))
        {
            texture->upload(data, data_size, layer);
        }
    }

    std::vector<std::uint8_t> Texture::read_back() const
    {
        if (auto* texture = static_cast<rhi::Texture*>(get_validated_texture()))
        {
            const std::size_t data_size =
                settings_.width *
                settings_.height *
                settings_.depth *
                channels_for_format(settings_.format);

            std::vector<std::uint8_t> data(data_size);
            texture->read_back(data.data(), data_size, 0);
            return data;
        }

        return {};
    }

    bool Texture::save_to_file(const std::string& filepath) const
    {
        auto data = read_back();
        if (data.empty()) return false;

        if (settings_.type != TextureType::Texture2D)
        {
            detail::FileIO::write(filepath, data);
            return true;
        }

        const detail::ImageData image_data{
            settings_.width,
            settings_.height,
            channels_for_format(settings_.format),
            data.data()
        };

        const bool result = detail::ImageIO::write(filepath, image_data);

        if (!result)
        {
            Log::error("Failed to save texture to file: {}", filepath);
            return false;
        }

        return true;
    }

    void* Texture::rhi_handle() const
    {
        if (rhi_textures_.empty()) return nullptr;

        const std::uint32_t frame_index = rhi::RenderContext::swapchain()->current_frame();
        const std::uint32_t texture_index = settings_.access_mode == ResourceAccessMode::Dynamic ? frame_index : 0;

        if (texture_index >= rhi_textures_.size())
        {
            Log::error("Invalid frame index {} for texture with {} textures", frame_index, rhi_textures_.size());
            return nullptr;
        }

        return rhi_textures_[texture_index];
    }

    void* Texture::get_validated_texture() const
    {
        if (rhi_textures_.empty())
        {
            Log::error("Cannot access invalid texture");
            return nullptr;
        }

        return rhi_handle();
    }


    void Texture::transition_layout(
        const TextureLayout old_layout,
        const TextureLayout new_layout) const
    {
        if (auto* texture = static_cast<rhi::Texture*>(get_validated_texture()))
        {
            texture->transition_layout(old_layout, new_layout);
        }
    }
}