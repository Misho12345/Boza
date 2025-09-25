#pragma once
#include "boza/std_pch.hpp"
#include "GraphicsObject.hpp"

namespace boza::rhi
{
    enum class TextureFormat : uint8_t
    {
        RGBA8,
        BGRA8,
        RGBA16F,
        RGBA32F,
        DEPTH24STENCIL8,
        DEPTH32F
    };

    enum class TextureUsage : uint8_t
    {
        Sampled,
        Storage,
        ColorAttachment,
        DepthStencilAttachment,
        TransferSrc,
        TransferDst,
        InputAttachment
    };

    struct TextureDesc
    {
        Device* device;

        uint32_t width;
        uint32_t height;

        TextureFormat format;
        TextureUsage  usage;
    };

    class Texture : public GraphicsObject<Texture, TextureDesc>
    {
    protected:
        explicit Texture(const TextureDesc& desc) : GraphicsObject(desc) {}
    };
}
