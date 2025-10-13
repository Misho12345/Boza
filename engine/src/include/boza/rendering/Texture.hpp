#pragma once
#include "boza/API.hpp"
#include "boza/core/Property.hpp"
#include <memory>
#include <string>

namespace boza
{
    enum class GraphicsApi;
    namespace rhi { class Device; }

    class BOZA_API Texture
    {
    public:
        enum class Format
        {
            R8,
            RG8,
            RGB8,
            RGBA8,
            BGRA8,
            R16F,
            RG16F,
            RGB16F,
            RGBA16F,
            R32F,
            RG32F,
            RGB32F,
            RGBA32F,
            DEPTH24STENCIL8,
            DEPTH32F
        };

        enum class Usage
        {
            Sampled,
            Storage,
            ColorAttachment,
            DepthStencilAttachment,
            TransferSrc,
            TransferDst,
            InputAttachment
        };

        struct Descriptor
        {
            uint32_t width  = 1;
            uint32_t height = 1;
            Format   format = Format::RGBA8;
            Usage    usage  = Usage::Sampled;
        };

        Texture(GraphicsApi api, rhi::Device* device, const Descriptor& desc);
        ~Texture();

        Texture(const Texture&)            = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        Texture& operator=(Texture&&) noexcept;

        void upload(const void* data, size_t size) const;
        bool load_from_file(const std::string& filepath) const;

        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4251)
        #endif

        PropertyGet<uint32_t> width{ GET { return get_width(); } };
        PropertyGet<uint32_t> height{ GET { return get_height(); } };

        PropertyGet<Format> format{ GET { return get_format(); } };
        PropertyGet<bool>   valid{ GET { return is_valid(); } };

        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif

        void* get_rhi_texture_internal() const;

    private:
        [[nodiscard]] uint32_t get_width() const;
        [[nodiscard]] uint32_t get_height() const;
        [[nodiscard]] Format get_format() const;
        [[nodiscard]] bool   is_valid() const;

        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4251)
        #endif

        struct Impl;
        std::unique_ptr<Impl> impl_;

        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif
    };
}
