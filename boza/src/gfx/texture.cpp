module boza.gfx;

import :texture;
import :buffer;

import boza.common;
import boza.core;

import boza.detail;

import boza.rhi;
import boza.rhi;

import :texture_loader;

namespace boza
{
    void* TextureAccess::handle(const Texture& texture) { return texture.rhi_handle(); }
    void* TextureAccess::handle(const Texture& texture, const std::uint32_t frame_index)
    {
        return texture.rhi_handle(frame_index);
    }

    TextureLayout TextureAccess::layout(const Texture& texture)
    {
        if (texture.layouts_.empty()) return TextureLayout::Undefined;

        if (texture.settings_.access_mode != ResourceAccessMode::Dynamic)
            return texture.layouts_.front();

        const auto* swapchain = rhi::RenderContext::swapchain();
        const std::uint32_t frame_index = swapchain ? swapchain->current_frame() : 0u;
        return texture.layouts_[frame_index % texture.layouts_.size()];
    }

    void TextureAccess::set_layout(Texture& texture, const TextureLayout layout)
    {
        if (texture.layouts_.empty()) return;

        if (texture.settings_.access_mode != ResourceAccessMode::Dynamic)
        {
            texture.layouts_.front() = layout;
            return;
        }

        const auto* swapchain = rhi::RenderContext::swapchain();
        const std::uint32_t frame_index = swapchain ? swapchain->current_frame() : 0u;
        texture.layouts_[frame_index % texture.layouts_.size()] = layout;
    }

    constexpr std::uint8_t channels_for_format(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::Undefined: return 4;
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

    constexpr std::size_t bytes_per_pixel_for_format(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::Undefined: return 4;
            case TextureFormat::R8: return 1;
            case TextureFormat::RG8: return 2;
            case TextureFormat::RGB8: return 3;
            case TextureFormat::RGBA8:
            case TextureFormat::BGRA8: return 4;

            case TextureFormat::R16F: return 2;
            case TextureFormat::RG16F: return 4;
            case TextureFormat::RGB16F: return 6;
            case TextureFormat::RGBA16F: return 8;

            case TextureFormat::R32F: return 4;
            case TextureFormat::RG32F: return 8;
            case TextureFormat::RGB32F: return 12;
            case TextureFormat::RGBA32F: return 16;

            case TextureFormat::DEPTH24STENCIL8: return 4;
            case TextureFormat::DEPTH32F: return 4;

            default: return 4;
        }
    }

    constexpr std::uint8_t bytes_per_channel_for_format(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R8:
            case TextureFormat::RG8:
            case TextureFormat::RGB8:
            case TextureFormat::RGBA8:
            case TextureFormat::BGRA8: return 1;

            case TextureFormat::R16F:
            case TextureFormat::RG16F:
            case TextureFormat::RGB16F:
            case TextureFormat::RGBA16F: return 2;

            case TextureFormat::R32F:
            case TextureFormat::RG32F:
            case TextureFormat::RGB32F:
            case TextureFormat::RGBA32F: return 4;

            case TextureFormat::DEPTH24STENCIL8:
            case TextureFormat::DEPTH32F: return 4;
            default: return 1;
        }
    }

    constexpr std::uint32_t depth_extent_for_copy(const TextureType type, const std::uint32_t depth)
    {
        if (type == TextureType::Texture3D) return std::max(depth, 1u);
        return 1u;
    }

    constexpr std::size_t copy_size_for_settings(const TextureSettings& settings)
    {
        return
            static_cast<std::size_t>(settings.width) *
            static_cast<std::size_t>(settings.height) *
            static_cast<std::size_t>(depth_extent_for_copy(settings.type, settings.depth)) *
            bytes_per_pixel_for_format(settings.format);
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
        layouts_.assign(texture_count, TextureLayout::Undefined);

        const bool is_cube =
            settings_.type == TextureType::TextureCube ||
            settings_.type == TextureType::TextureCubeArray;

        const bool is_array =
            settings_.type == TextureType::Texture1DArray ||
            settings_.type == TextureType::Texture2DArray ||
            settings_.type == TextureType::TextureCubeArray;

        const std::uint32_t image_depth = settings_.type == TextureType::Texture3D
                                             ? std::max(settings_.depth, 1u)
                                             : 1u;
        const std::uint32_t array_layers = is_array ? std::max(settings_.depth, 1u) : 1u;

        for (std::uint32_t i = 0; i < texture_count; ++i)
        {
            auto rhi_texture = create_texture(
                rhi::RenderContext::api(), {
                    .device       = rhi::RenderContext::device(),
                    .type         = settings_.type,
                    .format       = settings_.format,
                    .usage        = settings_.usage_flags,
                    .width        = settings_.width,
                    .height       = settings_.height,
                    .depth        = image_depth,
                    .array_layers = is_cube ? std::max(settings_.depth, 1u) : array_layers,
                });

            if (!rhi_texture)
            {
                Log::error("Failed to create texture {} of {} ({}x{}x{})",
                    i,
                    texture_count,
                    settings_.width,
                    settings_.height,
                    settings_.depth);
                cleanup();
                return;
            }

            rhi_textures_.emplace_back(rhi_texture.release(), &Texture::destroy_rhi_texture);
        }
    }

    Texture::~Texture() { cleanup(); }

    void Texture::cleanup()
    {
        rhi_textures_.clear();
        layouts_.clear();
    }

    Texture::Texture(Texture&& other) noexcept
        : name_{ std::move(other.name_) },
          settings_{ std::exchange(other.settings_, {}) },
          rhi_textures_{ std::move(other.rhi_textures_) },
          layouts_{ std::move(other.layouts_) } {}

    Texture& Texture::operator=(Texture&& other) noexcept
    {
        if (this == &other) return *this;

        cleanup();

        name_ = std::move(other.name_);
        rhi_textures_ = std::move(other.rhi_textures_);
        layouts_ = std::move(other.layouts_);
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

    void Texture::upload_from(const Buffer& staging_buffer, const std::uint32_t layer, const std::size_t size) const
    {
        auto* texture = static_cast<rhi::Texture*>(get_validated_texture());
        auto* staging = static_cast<rhi::Buffer*>(staging_buffer.rhi_handle());
        if (!texture || !staging)
        {
            Log::error("Cannot upload texture from invalid staging buffer");
            return;
        }

        const std::size_t transfer_size = size > 0 ? size : staging_buffer.size;
        if (!texture->upload_from(staging, transfer_size, layer))
        {
            Log::error("Failed to upload texture from staging buffer");
        }
    }

    Buffer Texture::stage(const std::size_t size, const std::uint32_t layer) const
    {
        const std::size_t default_size = copy_size_for_settings(settings_);

        const std::size_t stage_size = size > 0 ? size : default_size;

        if (rhi_textures_.empty())
        {
            Log::error("Cannot stage an invalid texture");
            return Buffer(stage_size, BufferUsage::Staging, ResourceAccessMode::Static);
        }

        std::vector<Buffer::RhiBufferHandle> staged_rhi_buffers;
        staged_rhi_buffers.reserve(rhi_textures_.size());

        for (const auto& handle : rhi_textures_)
        {
            auto* source_texture = static_cast<rhi::Texture*>(handle.get());
            if (!source_texture)
            {
                Log::error("Cannot stage invalid texture");
                staged_rhi_buffers.clear();
                break;
            }

            auto staged = source_texture->stage(stage_size, layer);
            if (!staged)
            {
                Log::error("Failed to stage texture data for layer {}", layer);
                staged_rhi_buffers.clear();
                break;
            }

            staged_rhi_buffers.emplace_back(staged.release(), &Buffer::destroy_rhi_buffer);
        }

        if (staged_rhi_buffers.size() == rhi_textures_.size())
        {
            return Buffer(std::move(staged_rhi_buffers), stage_size, settings_.access_mode);
        }

        Buffer fallback_staging_buffer{
            stage_size,
            BufferUsage::Staging,
            ResourceAccessMode::Static
        };

        if (void* mapped_data = fallback_staging_buffer.map())
        {
            if (auto* texture = static_cast<rhi::Texture*>(get_validated_texture()))
            {
                texture->read_back(mapped_data, fallback_staging_buffer.size, layer);
            }
            fallback_staging_buffer.unmap();
        }

        return fallback_staging_buffer;
    }

    std::vector<std::uint8_t> Texture::read_back() const
    {
        if (auto* texture = static_cast<rhi::Texture*>(get_validated_texture()))
        {
            const std::size_t data_size = copy_size_for_settings(settings_);

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
            bytes_per_channel_for_format(settings_.format),
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

        if (settings_.access_mode != ResourceAccessMode::Dynamic) return rhi_textures_.front().get();

        const auto* swapchain = rhi::RenderContext::swapchain();
        if (!swapchain)
        {
            Log::error("Cannot resolve dynamic texture handle: swapchain is null");
            return nullptr;
        }

        return rhi_handle(swapchain->current_frame());
    }

    void* Texture::rhi_handle(const std::uint32_t frame_index) const
    {
        if (rhi_textures_.empty()) return nullptr;
        if (settings_.access_mode != ResourceAccessMode::Dynamic) return rhi_textures_.front().get();

        if (frame_index >= rhi_textures_.size())
        {
            Log::error("Invalid frame index {} for texture with {} textures", frame_index, rhi_textures_.size());
            return nullptr;
        }

        return rhi_textures_[frame_index].get();
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

    void Texture::destroy_rhi_texture(void* handle)
    {
        delete static_cast<rhi::Texture*>(handle);
    }

    void Texture::transition_layout(
        const TextureLayout old_layout,
        const TextureLayout new_layout) const
    {
        if (auto* texture = static_cast<rhi::Texture*>(get_validated_texture()))
        {
            texture->transition_layout(old_layout, new_layout);
            const_cast<Texture*>(this)->layouts_.assign(layouts_.size(), new_layout);
        }
    }
}
