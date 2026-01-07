module;

#include <cassert>

module boza.gfx;

import :sampler;
import boza.rhi;
import boza.core;
import boza.detail;
import boza.gfx.sampler_loader;

namespace boza
{
    Sampler::Sampler(
        const std::string_view name,
        const SamplerFilter filter,
        const SamplerWrap wrap_u,
        const SamplerWrap wrap_v,
        const SamplerWrap wrap_w,
        const SamplerFilter mipmap_mode,
        const float mip_lod_bias,
        const float min_lod,
        const float max_lod,
        const float max_anisotropy)
        : name_{ name },
          filter_{ filter },
          wrap_u_{ wrap_u },
          wrap_v_{ wrap_v },
          wrap_w_{ wrap_w },
          mipmap_mode_{ mipmap_mode },
          mip_lod_bias_{ mip_lod_bias },
          min_lod_{ min_lod },
          max_lod_{ max_lod },
          max_anisotropy_{ max_anisotropy }
    {
        if (!detail::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return;
        }

        rhi_sampler_ = create_sampler(
            detail::RenderContext::api(), {
                .device = detail::RenderContext::device(),
                .type = TextureType::Texture2D,
                .filter = filter_,
                .wrap_u = wrap_u_,
                .wrap_v = wrap_v_,
                .wrap_w = wrap_w_,
                .mipmap_mode = mipmap_mode_,
                .mip_lod_bias = mip_lod_bias_,
                .min_lod = min_lod_,
                .max_lod = max_lod_,
                .max_anisotropy = max_anisotropy_
            });

        if (!rhi_sampler_) Log::error("Failed to create sampler: {}", name_);
    }

    Sampler::~Sampler()
    {
        if (!rhi_sampler_) return;

        auto* sampler = static_cast<rhi::Sampler*>(rhi_sampler_);
        sampler->destroy();
        delete sampler;
    }

    Sampler::Sampler(Sampler&& other) noexcept
        :    name_{ std::move(other.name_) },
          filter_{ other.filter_ },
          wrap_u_{ other.wrap_u_ },
          wrap_v_{ other.wrap_v_ },
          wrap_w_{ other.wrap_w_ },
          mipmap_mode_{ other.mipmap_mode_ },
          mip_lod_bias_{ other.mip_lod_bias_ },
          min_lod_{ other.min_lod_ },
          max_lod_{ other.max_lod_ },
          max_anisotropy_{ other.max_anisotropy_ },
          rhi_sampler_{ std::exchange(other.rhi_sampler_, nullptr) } {}

    Sampler& Sampler::operator=(Sampler&& other) noexcept
    {
        if (this == &other) return *this;

        if (rhi_sampler_)
        {
            auto* sampler = static_cast<rhi::Sampler*>(rhi_sampler_);
            sampler->destroy();
            delete sampler;
        }

        name_           = std::move(other.name_);
        filter_         = other.filter_;
        wrap_u_         = other.wrap_u_;
        wrap_v_         = other.wrap_v_;
        wrap_w_         = other.wrap_w_;
        mipmap_mode_    = other.mipmap_mode_;
        mip_lod_bias_   = other.mip_lod_bias_;
        min_lod_        = other.min_lod_;
        max_lod_        = other.max_lod_;
        max_anisotropy_ = other.max_anisotropy_;
        rhi_sampler_    = std::exchange(other.rhi_sampler_, nullptr);

        return *this;
    }

    Sampler& Sampler::create(
        const std::string_view name,
        const SamplerFilter filter,
        const SamplerWrap wrap_u,
        const SamplerWrap wrap_v,
        const SamplerWrap wrap_w,
        const SamplerFilter mipmap_mode,
        const float mip_lod_bias,
        const float min_lod,
        const float max_lod,
        const float max_anisotropy)
    {
        return gfx::SamplerLoader::instance().create(
            name, filter, wrap_u, wrap_v, wrap_w, mipmap_mode, mip_lod_bias, min_lod, max_lod, max_anisotropy);
    }

    Sampler& Sampler::get(const std::string_view name) { return gfx::SamplerLoader::instance().get_sampler(name); }

    Sampler* Sampler::try_get(const std::string_view name)
    {
        return gfx::SamplerLoader::instance().try_get_sampler(name);
    }

    void Sampler::destroy(std::string_view name)
    {
        gfx::SamplerLoader::instance().destroy(name);
    }
}
