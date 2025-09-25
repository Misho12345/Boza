#pragma once
#include "boza/std_pch.hpp"
#include "GraphicsObject.hpp"

namespace boza::rhi
{
    enum class SamplerFilter : uint8_t
    {
        Nearest,
        Linear,
        Anisotropic
    };

    enum class SamplerAddressMode : uint8_t
    {
        Repeat,
        ClampToEdge,
        Mirror
    };

    struct SamplerDesc
    {
        Device*         device;
        SamplerFilter      filter;
        SamplerAddressMode address_mode_u    = SamplerAddressMode::Repeat;
        SamplerAddressMode address_mode_v    = SamplerAddressMode::Repeat;
        SamplerAddressMode address_mode_w    = SamplerAddressMode::Repeat;
        float              min_lod           = 0.0f;
        float              max_lod           = 0.0f;
        float              mip_lod_bias      = 0.0f;
        float              anisotropy_enable = 0.0f;
        float              max_anisotropy    = 0.0f;
        bool               compare_enable    = false;
        float              compare_op        = 0.0f;
    };

    class Sampler : public GraphicsObject<Sampler, SamplerDesc>
    {
    protected:
        explicit Sampler(const SamplerDesc& desc) : GraphicsObject(desc) {}
    };
}
