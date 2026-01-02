module boza.gfx;

import :sampler;
import boza.rhi;
import boza.core;
import boza.detail;
import boza.gfx.sampler_loader;

namespace boza
{
    Sampler::Sampler(
        const SamplerFilter filter,
        const SamplerWrap wrap_u,
        const SamplerWrap wrap_v,
        const SamplerWrap wrap_w,
        const SamplerFilter mipmap_mode,
        const float mip_lod_bias,
        const float min_lod,
        const float max_lod,
        const float max_anisotropy)
        : filter_{ filter },
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

        if (!rhi_sampler_) Log::error("Failed to create sampler.");
    }

    Sampler::~Sampler()
    {
        if (!rhi_sampler_) return;

        auto* sampler = static_cast<rhi::Sampler*>(rhi_sampler_);
        sampler->destroy();
        delete sampler;
    }

    Sampler* Sampler::create(
        const std::string& name,
        const SamplerFilter filter,
        const SamplerWrap wrap)
    {
        return create_internal(name, filter, wrap, wrap, wrap, filter, 0.0f, 0.0f, 1000.0f, 1.0f);
    }

    Sampler* Sampler::create_internal(
        const std::string& name,
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
        auto* sampler = new Sampler(
            filter, wrap_u, wrap_v, wrap_w,
            mipmap_mode, mip_lod_bias, min_lod, max_lod, max_anisotropy);

        if (!sampler->rhi_handle())
        {
            Log::error("Failed to create Sampler");
            delete sampler;
            return nullptr;
        }

        gfx::SamplerLoader::instance().register_sampler(name, sampler, true);
        return sampler;
    }

    Sampler* Sampler::get(const std::string& name)
    {
        return gfx::SamplerLoader::instance().get_or_load(name);
    }

    void Sampler::destroy(Sampler* sampler)
    {
        if (!sampler) return;

        gfx::SamplerLoader::instance().unregister_sampler(sampler);
        delete sampler;
    }
}