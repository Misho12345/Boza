#include "boza/rendering/Texture.hpp"
#include "boza/core/Logger.hpp"
#include "boza/rhi/Factory.hpp"
#include "boza/rhi/Resources.hpp"
#include "boza/GraphicsApi.hpp"

namespace boza
{
    struct Texture::Impl
    {
        std::unique_ptr<rhi::Texture> rhi_texture;
        Descriptor                    descriptor;
        GraphicsApi                   api;
        rhi::Device*                  device;
        bool                          is_valid = false;

        static rhi::TextureFormat convert_format(const Format format)
        {
            switch (format)
            {
                case Format::R8: return rhi::TextureFormat::R8;
                case Format::RG8: return rhi::TextureFormat::RG8;
                case Format::RGB8: return rhi::TextureFormat::RGB8;
                case Format::RGBA8: return rhi::TextureFormat::RGBA8;
                case Format::BGRA8: return rhi::TextureFormat::BGRA8;
                case Format::R16F: return rhi::TextureFormat::R16F;
                case Format::RG16F: return rhi::TextureFormat::RG16F;
                case Format::RGB16F: return rhi::TextureFormat::RGB16F;
                case Format::RGBA16F: return rhi::TextureFormat::RGBA16F;
                case Format::R32F: return rhi::TextureFormat::R32F;
                case Format::RG32F: return rhi::TextureFormat::RG32F;
                case Format::RGB32F: return rhi::TextureFormat::RGB32F;
                case Format::RGBA32F: return rhi::TextureFormat::RGBA32F;
                case Format::DEPTH24STENCIL8: return rhi::TextureFormat::DEPTH24STENCIL8;
                case Format::DEPTH32F: return rhi::TextureFormat::DEPTH32F;
                default: return rhi::TextureFormat::RGBA8;
            }
        }

        static rhi::TextureUsage convert_usage(const Usage usage)
        {
            switch (usage)
            {
                case Usage::Sampled: return rhi::TextureUsage::Sampled;
                case Usage::Storage: return rhi::TextureUsage::Storage;
                case Usage::ColorAttachment: return rhi::TextureUsage::ColorAttachment;
                case Usage::DepthStencilAttachment: return rhi::TextureUsage::DepthStencilAttachment;
                case Usage::TransferSrc: return rhi::TextureUsage::TransferSrc;
                case Usage::TransferDst: return rhi::TextureUsage::TransferDst;
                case Usage::InputAttachment: return rhi::TextureUsage::InputAttachment;
                default: return rhi::TextureUsage::Sampled;
            }
        }

        bool create_rhi_texture()
        {
            const rhi::TextureDesc desc
            {
                .device = device,
                .width = descriptor.width,
                .height = descriptor.height,
                .format = convert_format(descriptor.format),
                .usage = convert_usage(descriptor.usage)
            };

            rhi_texture.reset(rhi::create_texture(api, desc));
            return rhi_texture != nullptr;
        }
    };

    Texture::Texture(const GraphicsApi api, rhi::Device* device, const Descriptor& desc)
        : impl_(std::make_unique<Impl>())
    {
        impl_->descriptor = desc;
        impl_->api        = api;
        impl_->device     = device;
        impl_->is_valid   = false;

        if (!impl_->create_rhi_texture()) Logger::error("Failed to create RHI texture");
    }

    Texture::~Texture() = default;

    Texture::Texture(Texture&&) noexcept            = default;
    Texture& Texture::operator=(Texture&&) noexcept = default;

    void Texture::upload(const void* data, const size_t size) const
    {
        if (!impl_->rhi_texture)
        {
            Logger::error("Cannot upload to texture - RHI texture not initialized");
            return;
        }

        impl_->rhi_texture->upload(data, size);
    }

    bool Texture::load_from_file(const std::string& filepath) const
    {
        if (!impl_->rhi_texture)
        {
            Logger::error("Cannot load texture from file - RHI texture not initialized");
            return false;
        }

        const bool result = impl_->rhi_texture->load_from_file(filepath);

        if (result)
        {
            impl_->descriptor.width  = impl_->rhi_texture->width();
            impl_->descriptor.height = impl_->rhi_texture->height();
            impl_->is_valid          = true;
        }

        return result;
    }

    uint32_t Texture::get_width() const { return impl_->descriptor.width; }
    uint32_t Texture::get_height() const { return impl_->descriptor.height; }

    Texture::Format Texture::get_format() const { return impl_->descriptor.format; }
    bool            Texture::is_valid() const { return impl_->is_valid; }

    void* Texture::get_rhi_texture_internal() const { return impl_->rhi_texture.get(); }
}
