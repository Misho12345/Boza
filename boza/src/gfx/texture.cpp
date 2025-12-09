module boza.gfx;

import :texture;
import boza.rhi;
import boza.common;
import boza.core;
import boza.detail;

namespace boza
{
    using detail::ImageIO;
    using detail::ImageData;

    rhi::TextureFormat to_rhi_format(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R8: return rhi::TextureFormat::R8;
            case TextureFormat::RG8: return rhi::TextureFormat::RG8;
            case TextureFormat::RGB8: return rhi::TextureFormat::RGB8;
            case TextureFormat::RGBA8: return rhi::TextureFormat::RGBA8;
            case TextureFormat::BGRA8: return rhi::TextureFormat::BGRA8;
            case TextureFormat::R16F: return rhi::TextureFormat::R16F;
            case TextureFormat::RG16F: return rhi::TextureFormat::RG16F;
            case TextureFormat::RGB16F: return rhi::TextureFormat::RGB16F;
            case TextureFormat::RGBA16F: return rhi::TextureFormat::RGBA16F;
            case TextureFormat::R32F: return rhi::TextureFormat::R32F;
            case TextureFormat::RG32F: return rhi::TextureFormat::RG32F;
            case TextureFormat::RGB32F: return rhi::TextureFormat::RGB32F;
            case TextureFormat::RGBA32F: return rhi::TextureFormat::RGBA32F;
            case TextureFormat::DEPTH24STENCIL8: return rhi::TextureFormat::DEPTH24STENCIL8;
            case TextureFormat::DEPTH32F: return rhi::TextureFormat::DEPTH32F;
        }
        return rhi::TextureFormat::RGBA8;
    }

    rhi::SamplerFilter to_rhi_filter(const SamplerFilter filter)
    {
        switch (filter)
        {
            case SamplerFilter::Nearest: return rhi::SamplerFilter::Nearest;
            case SamplerFilter::Linear: return rhi::SamplerFilter::Linear;
            case SamplerFilter::Anisotropic: return rhi::SamplerFilter::Anisotropic;
        }
        return rhi::SamplerFilter::Linear;
    }

    rhi::SamplerAddressMode to_rhi_wrap(const SamplerWrap wrap)
    {
        switch (wrap)
        {
            case SamplerWrap::Repeat: return rhi::SamplerAddressMode::Repeat;
            case SamplerWrap::ClampToEdge: return rhi::SamplerAddressMode::ClampToEdge;
            case SamplerWrap::Mirror: return rhi::SamplerAddressMode::Mirror;
        }
        return rhi::SamplerAddressMode::Repeat;
    }

    Flags<rhi::TextureUsage> to_rhi_usage(const std::uint32_t usage_flags)
    {
        Flags<rhi::TextureUsage> flags;
        if (usage_flags & static_cast<std::uint32_t>(TextureUsage::Sampled)) flags |= rhi::TextureUsage::Sampled;
        if (usage_flags & static_cast<std::uint32_t>(TextureUsage::Storage)) flags |= rhi::TextureUsage::Storage;
        if (usage_flags & static_cast<std::uint32_t>(TextureUsage::ColorAttachment)) flags |= rhi::TextureUsage::ColorAttachment;
        if (usage_flags & static_cast<std::uint32_t>(TextureUsage::DepthStencilAttachment)) flags |= rhi::TextureUsage::DepthStencilAttachment;
        if (usage_flags & static_cast<std::uint32_t>(TextureUsage::TransferSrc)) flags |= rhi::TextureUsage::TransferSrc;
        if (usage_flags & static_cast<std::uint32_t>(TextureUsage::TransferDst)) flags |= rhi::TextureUsage::TransferDst;
        if (usage_flags & static_cast<std::uint32_t>(TextureUsage::InputAttachment)) flags |= rhi::TextureUsage::InputAttachment;
        return flags;
    }


    Texture::Texture(
        const std::uint32_t     texture_width,
        const std::uint32_t     texture_height,
        const TextureFormat     texture_format,
        const std::uint32_t     usage_flags,
        const TextureAccessMode texture_access_mode)
        : rhi_sampler_(nullptr),
          width_(texture_width),
          height_(texture_height),
          format_(texture_format),
          access_mode_(texture_access_mode)
    {
        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return;
        }

        const std::uint32_t texture_count =
                texture_access_mode == TextureAccessMode::Dynamic ? detail::RenderContext::frames_in_flight() : 1;

        rhi_textures_.reserve(texture_count);

        for (std::uint32_t i = 0; i < texture_count; ++i)
        {
            void* rhi_texture = rhi::create_texture(
                static_cast<rhi::GraphicsApi>(detail::RenderContext::api()), {
                    .device = static_cast<rhi::Device*>(detail::RenderContext::device()),
                    .width = texture_width,
                    .height = texture_height,
                    .format = to_rhi_format(texture_format),
                    .usage = to_rhi_usage(usage_flags),
                    .access_mode = rhi::ResourceAccessMode::Static
                });

            if (!rhi_texture)
            {
                Log::error("Failed to create texture {} of {} ({}x{})", i, texture_count, texture_width,
                           texture_height);
                for (auto* tex : rhi_textures_)
                {
                    auto* texture = static_cast<rhi::Texture*>(tex);
                    texture->destroy();
                    delete texture;
                }
                rhi_textures_.clear();
                return;
            }

            rhi_textures_.push_back(rhi_texture);
        }

        rhi_sampler_ = rhi::create_sampler(
            static_cast<rhi::GraphicsApi>(detail::RenderContext::api()), {
                .device = static_cast<rhi::Device*>(detail::RenderContext::device()),
                .filter = rhi::SamplerFilter::Linear,
                .address_mode_u = rhi::SamplerAddressMode::Repeat,
                .address_mode_v = rhi::SamplerAddressMode::Repeat,
                .address_mode_w = rhi::SamplerAddressMode::Repeat
            });

        if (!rhi_sampler_) { Log::error("Failed to create sampler for texture"); }
    }

    Texture::~Texture()
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

        if (rhi_sampler_)
        {
            auto* sampler = static_cast<rhi::Sampler*>(rhi_sampler_);
            sampler->destroy();
            delete sampler;
            rhi_sampler_ = nullptr;
        }
    }

    Texture::Texture(Texture&& other) noexcept
        : rhi_textures_(std::move(other.rhi_textures_)),
          rhi_sampler_(other.rhi_sampler_),
          width_(other.width_),
          height_(other.height_),
          format_(other.format_),
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
            for (auto* rhi_texture : rhi_textures_)
            {
                if (rhi_texture)
                {
                    auto* texture = static_cast<rhi::Texture*>(rhi_texture);
                    texture->destroy();
                    delete texture;
                }
            }
            if (rhi_sampler_)
            {
                auto* sampler = static_cast<rhi::Sampler*>(rhi_sampler_);
                sampler->destroy();
                delete sampler;
            }

            rhi_textures_ = std::move(other.rhi_textures_);
            rhi_sampler_  = other.rhi_sampler_;
            width_        = other.width_;
            height_       = other.height_;
            format_       = other.format_;
            access_mode_  = other.access_mode_;

            other.rhi_sampler_ = nullptr;
            other.width_       = 0;
            other.height_      = 0;
        }
        return *this;
    }

    constexpr int channels_for_format(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R8:
            case TextureFormat::R16F:
            case TextureFormat::R32F:
                return 1;
            case TextureFormat::RG8:
            case TextureFormat::RG16F:
            case TextureFormat::RG32F:
                return 2;
            case TextureFormat::RGB8:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB32F:
                return 3;
            case TextureFormat::RGBA8:
            case TextureFormat::BGRA8:
            case TextureFormat::RGBA16F:
            case TextureFormat::RGBA32F:
                return 4;
            default:
                return 4;
        }
    }

    Texture* Texture::load_from_file(
        const std::string&      filepath,
        const TextureFormat     texture_format,
        const TextureAccessMode texture_access_mode)
    {
        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return nullptr;
        }

        const int desired_channels = channels_for_format(texture_format);
        const ImageData image_data = ImageIO::read(filepath, desired_channels);
        if (!image_data.data)
        {
            Log::error("Failed to load image from file: {}", filepath);
            return nullptr;
        }

        constexpr std::uint32_t usage_flags =
                static_cast<std::uint32_t>(TextureUsage::Sampled) |
                static_cast<std::uint32_t>(TextureUsage::TransferDst);

        auto* texture = new Texture(
            image_data.width,
            image_data.height,
            texture_format,
            usage_flags,
            texture_access_mode);

        if (texture->rhi_textures_.empty())
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

    void Texture::upload(const void* data, const std::size_t data_size, const std::uint32_t frame_index) const
    {
        if (rhi_textures_.empty())
        {
            Log::error("Cannot upload to null texture");
            return;
        }

        const std::uint32_t texture_index =
                access_mode_ == TextureAccessMode::Dynamic ? frame_index : 0;

        if (texture_index >= rhi_textures_.size())
        {
            Log::error("Invalid frame index {} for texture with {} textures", frame_index, rhi_textures_.size());
            return;
        }

        static_cast<rhi::Texture*>(rhi_textures_[texture_index])->upload(data, data_size);
    }

    bool Texture::save_to_file(const std::string& filepath, const std::uint32_t frame_index) const
    {
        if (rhi_textures_.empty())
        {
            Log::error("Cannot save null texture to file");
            return false;
        }

        const std::uint32_t texture_index =
                access_mode_ == TextureAccessMode::Dynamic ? frame_index : 0;

        if (texture_index >= rhi_textures_.size())
        {
            Log::error("Invalid frame index {} for texture with {} textures", frame_index, rhi_textures_.size());
            return false;
        }

        const std::size_t data_size  = width_ * height_ * 4;
        const auto        pixel_data = std::make_unique<std::uint8_t[]>(data_size);

        static_cast<rhi::Texture*>(rhi_textures_[texture_index])->download(pixel_data.get(), data_size);

        ImageData image_data{
            width_,
            height_,
            4,
            pixel_data.get()
        };

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
        if (rhi_textures_.empty()) { return nullptr; }

        const std::uint32_t texture_index =
                access_mode_ == TextureAccessMode::Dynamic ? frame_index : 0;

        if (texture_index >= rhi_textures_.size())
        {
            Log::error("Invalid frame index {} for texture with {} textures", frame_index, rhi_textures_.size());
            return nullptr;
        }

        return rhi_textures_[texture_index];
    }

    void Texture::set_sampler_filter(const SamplerFilter filter)
    {
        if (!rhi_sampler_)
        {
            Log::warn("Cannot set filter on null sampler");
            return;
        }

        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized");
            return;
        }

        auto* old_sampler = static_cast<rhi::Sampler*>(rhi_sampler_);
        old_sampler->destroy();
        delete old_sampler;

        rhi_sampler_ = rhi::create_sampler(
            static_cast<rhi::GraphicsApi>(detail::RenderContext::api()), {
                .device = static_cast<rhi::Device*>(detail::RenderContext::device()),
                .filter = to_rhi_filter(filter),
                .address_mode_u = rhi::SamplerAddressMode::Repeat,
                .address_mode_v = rhi::SamplerAddressMode::Repeat,
                .address_mode_w = rhi::SamplerAddressMode::Repeat
            });
    }

    void Texture::set_sampler_wrap(const SamplerWrap wrap_u, const SamplerWrap wrap_v, const SamplerWrap wrap_w)
    {
        if (!rhi_sampler_)
        {
            Log::warn("Cannot set wrap mode on null sampler");
            return;
        }

        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized");
            return;
        }

        auto* old_sampler = static_cast<rhi::Sampler*>(rhi_sampler_);
        old_sampler->destroy();
        delete old_sampler;

        rhi_sampler_ = rhi::create_sampler(
            static_cast<rhi::GraphicsApi>(detail::RenderContext::api()), {
                .device = static_cast<rhi::Device*>(detail::RenderContext::device()),
                .filter = rhi::SamplerFilter::Linear,
                .address_mode_u = to_rhi_wrap(wrap_u),
                .address_mode_v = to_rhi_wrap(wrap_v),
                .address_mode_w = to_rhi_wrap(wrap_w)
            });
    }

    void Texture::transition_layout(
        const TextureLayout old_layout,
        const TextureLayout new_layout,
        const std::uint32_t frame_index) const
    {
        if (rhi_textures_.empty())
        {
            Log::error("Cannot transition layout of null texture");
            return;
        }

        const std::uint32_t texture_index = access_mode_ == TextureAccessMode::Dynamic ? frame_index : 0;

        if (texture_index >= rhi_textures_.size())
        {
            Log::error("Invalid frame index {} for texture with {} textures", frame_index, rhi_textures_.size());
            return;
        }

        auto rhi_old_layout = rhi::TextureLayout::Undefined;
        auto rhi_new_layout = rhi::TextureLayout::Undefined;

        switch (old_layout)
        {
            case TextureLayout::Undefined: rhi_old_layout = rhi::TextureLayout::Undefined; break;
            case TextureLayout::General: rhi_old_layout = rhi::TextureLayout::General; break;
            case TextureLayout::ColorAttachment: rhi_old_layout = rhi::TextureLayout::ColorAttachment; break;
            case TextureLayout::DepthStencilAttachment: rhi_old_layout = rhi::TextureLayout::DepthStencilAttachment; break;
            case TextureLayout::ShaderReadOnly: rhi_old_layout = rhi::TextureLayout::ShaderReadOnly; break;
            case TextureLayout::TransferSrc: rhi_old_layout = rhi::TextureLayout::TransferSrc; break;
            case TextureLayout::TransferDst: rhi_old_layout = rhi::TextureLayout::TransferDst; break;
            case TextureLayout::Present: rhi_old_layout = rhi::TextureLayout::Present; break;
        }

        switch (new_layout)
        {
            case TextureLayout::Undefined: rhi_new_layout = rhi::TextureLayout::Undefined; break;
            case TextureLayout::General: rhi_new_layout = rhi::TextureLayout::General; break;
            case TextureLayout::ColorAttachment: rhi_new_layout = rhi::TextureLayout::ColorAttachment; break;
            case TextureLayout::DepthStencilAttachment: rhi_new_layout = rhi::TextureLayout::DepthStencilAttachment; break;
            case TextureLayout::ShaderReadOnly: rhi_new_layout = rhi::TextureLayout::ShaderReadOnly; break;
            case TextureLayout::TransferSrc: rhi_new_layout = rhi::TextureLayout::TransferSrc; break;
            case TextureLayout::TransferDst: rhi_new_layout = rhi::TextureLayout::TransferDst; break;
            case TextureLayout::Present: rhi_new_layout = rhi::TextureLayout::Present; break;
        }

        static_cast<rhi::Texture*>(rhi_textures_[texture_index])->transition_layout(rhi_old_layout, rhi_new_layout);
    }
}