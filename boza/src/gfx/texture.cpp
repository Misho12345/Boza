module boza.gfx;

import :texture;
import boza.rhi;
import boza.common;
import boza.core;
import boza.detail;
import boza.gfx.texture_loader;

namespace boza
{
    using detail::ImageIO;
    using detail::ImageData;

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

    std::uint32_t Texture::resolve_texture_index(const std::uint32_t frame_index) const
    {
        const std::uint32_t index = access_mode_ == ResourceAccessMode::Dynamic ? frame_index : 0;

        if (index >= rhi_textures_.size())
        {
            Log::error("Invalid frame index {} for texture with {} textures", frame_index, rhi_textures_.size());
            return 0;
        }

        return index;
    }

    void* Texture::create_rhi_sampler(const SamplerFilter filter, const SamplerWrap wrap)
    {
        return create_sampler(
            static_cast<rhi::GraphicsApi>(detail::RenderContext::api()), {
                .device = static_cast<rhi::Device*>(detail::RenderContext::device()),
                .filter = filter,
                .wrap_u = wrap,
                .wrap_v = wrap,
                .wrap_w = wrap
            });
    }

    void Texture::cleanup_textures()
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

    void Texture::cleanup_sampler()
    {
        if (rhi_sampler_)
        {
            auto* sampler = static_cast<rhi::Sampler*>(rhi_sampler_);
            sampler->destroy();
            delete sampler;
            rhi_sampler_ = nullptr;
        }
    }

    Texture::Texture(
        const std::uint32_t       width,
        const std::uint32_t       height,
        const TextureFormat       format,
        const Flags<TextureUsage> usage_flags,
        const ResourceAccessMode  access_mode,
        const SamplerFilter       filter,
        const SamplerWrap         wrap)
        : width_(width),
          height_(height),
          format_(format),
          filter_(filter),
          wrap_(wrap),
          access_mode_(access_mode)
    {
        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return;
        }

        const std::uint32_t texture_count = access_mode == ResourceAccessMode::Dynamic
                                                ? detail::RenderContext::frames_in_flight()
                                                : 1;

        rhi_textures_.reserve(texture_count);

        for (std::uint32_t i = 0; i < texture_count; ++i)
        {
            void* rhi_texture = create_texture(
                static_cast<rhi::GraphicsApi>(detail::RenderContext::api()), {
                    .device = static_cast<rhi::Device*>(detail::RenderContext::device()),
                    .width = width,
                    .height = height,
                    .format = format,
                    .usage = usage_flags,
                });

            if (!rhi_texture)
            {
                Log::error("Failed to create texture {} of {} ({}x{})", i, texture_count, width, height);
                cleanup_textures();
                return;
            }

            rhi_textures_.push_back(rhi_texture);
        }

        rhi_sampler_ = create_rhi_sampler(filter_, wrap_);
        if (!rhi_sampler_)
        {
            Log::error("Failed to create sampler for texture");
            cleanup_textures();
        }
    }

    Texture::~Texture()
    {
        cleanup_textures();
        cleanup_sampler();
    }

    Texture::Texture(Texture&& other) noexcept
        : rhi_textures_(std::move(other.rhi_textures_)),
          rhi_sampler_(other.rhi_sampler_),
          width_(other.width_),
          height_(other.height_),
          format_(other.format_),
          filter_(other.filter_),
          wrap_(other.wrap_),
          access_mode_(other.access_mode_)
    {
        other.rhi_sampler_ = nullptr;
        other.width_       = 0;
        other.height_      = 0;
    }

    Texture& Texture::operator=(Texture&& other) noexcept
    {
        if (this != &other)
        {
            cleanup_textures();
            cleanup_sampler();

            rhi_textures_ = std::move(other.rhi_textures_);
            rhi_sampler_  = other.rhi_sampler_;
            width_        = other.width_;
            height_       = other.height_;
            format_       = other.format_;
            filter_       = other.filter_;
            wrap_         = other.wrap_;
            access_mode_  = other.access_mode_;

            other.rhi_sampler_ = nullptr;
            other.width_       = 0;
            other.height_      = 0;
        }
        return *this;
    }

    Texture* Texture::get_or_load(
        const std::string&  filepath,
        const TextureFormat texture_format,
        const SamplerFilter filter,
        const SamplerWrap   wrap)
    {
        return gfx::TextureLoader::instance().get_or_load(filepath, texture_format, filter, wrap);
    }

    Texture* Texture::load_copy(
        const std::string&       filepath,
        const TextureFormat      texture_format,
        const ResourceAccessMode texture_access_mode,
        const SamplerFilter      filter,
        const SamplerWrap        wrap)
    {
        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return nullptr;
        }

        const auto      full_path        = detail::AssetPaths::texture(filepath);
        const int       desired_channels = channels_for_format(texture_format);
        const ImageData image_data       = ImageIO::read(full_path.string(), desired_channels);

        if (!image_data.data)
        {
            Log::error("Failed to load image from file: {}", filepath);
            return nullptr;
        }

        constexpr auto usage_flags = TextureUsage::Sampled | TextureUsage::TransferDst;

        auto* texture = new Texture(
            image_data.width,
            image_data.height,
            texture_format,
            usage_flags,
            texture_access_mode,
            filter,
            wrap);

        if (!texture->is_valid())
        {
            Log::error("Failed to create texture for file: {}", filepath);
            delete texture;
            return nullptr;
        }

        for (auto* rhi_tex : texture->rhi_textures_)
        {
            static_cast<rhi::Texture*>(rhi_tex)->upload(image_data.data, image_data.size);
        }

        return texture;
    }

    Texture* Texture::create(
        const std::string&        name,
        const std::uint32_t       texture_width,
        const std::uint32_t       texture_height,
        const TextureFormat       texture_format,
        const Flags<TextureUsage> usage_flags,
        const ResourceAccessMode  texture_access_mode,
        const SamplerFilter       filter,
        const SamplerWrap         wrap)
    {
        auto* texture = new Texture(
            texture_width,
            texture_height,
            texture_format,
            usage_flags,
            texture_access_mode,
            filter,
            wrap);

        if (!texture->is_valid())
        {
            Log::error("Failed to create Texture");
            delete texture;
            return nullptr;
        }

        gfx::TextureLoader::instance().register_texture(name, texture, true);
        return texture;
    }

    Texture* Texture::get(const std::string& name)
    {
        return gfx::TextureLoader::instance().get_texture(name);
    }

    void Texture::destroy(Texture* texture)
    {
        if (!texture) return;

        gfx::TextureLoader::instance().unregister_texture(texture);
        delete texture;
    }

    void Texture::upload(const void* data, const std::size_t data_size, const std::uint32_t frame_index) const
    {
        if (!is_valid())
        {
            Log::error("Cannot upload to invalid texture");
            return;
        }

        const std::uint32_t texture_index = resolve_texture_index(frame_index);
        static_cast<rhi::Texture*>(rhi_textures_[texture_index])->upload(data, data_size);
    }

    bool Texture::save_to_file(const std::string& filepath, const std::uint32_t frame_index) const
    {
        if (!is_valid())
        {
            Log::error("Cannot save invalid texture to file");
            return false;
        }

        const std::uint32_t texture_index = resolve_texture_index(frame_index);
        const std::size_t   data_size     = width_ * height_ * 4;
        const auto          pixel_data    = std::make_unique<std::uint8_t[]>(data_size);

        static_cast<rhi::Texture*>(rhi_textures_[texture_index])->download(pixel_data.get(), data_size);

        ImageData image_data{ width_, height_, 4, pixel_data.get() };
        const bool result = ImageIO::write(filepath, image_data);
        image_data.data = nullptr;

        if (!result)
        {
            Log::error("Failed to save texture to file: {}", filepath);
            return false;
        }

        return true;
    }

    void* Texture::rhi_handle(const std::uint32_t frame_index) const
    {
        if (!is_valid()) return nullptr;
        return rhi_textures_[resolve_texture_index(frame_index)];
    }

    void Texture::change_sampler(const SamplerFilter filter, const SamplerWrap wrap)
    {
        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized");
            return;
        }

        cleanup_sampler();

        filter_ = filter;
        wrap_   = wrap;

        rhi_sampler_ = create_rhi_sampler(filter_, wrap_);
        if (!rhi_sampler_)
        {
            Log::error("Failed to recreate sampler");
        }
    }

    void Texture::transition_layout(
        const TextureLayout old_layout,
        const TextureLayout new_layout,
        const std::uint32_t frame_index) const
    {
        if (!is_valid())
        {
            Log::error("Cannot transition layout of invalid texture");
            return;
        }

        const std::uint32_t texture_index = resolve_texture_index(frame_index);
        static_cast<rhi::Texture*>(rhi_textures_[texture_index])->transition_layout(old_layout, new_layout);
    }
}