module boza.gfx;

import :sampler;

import boza.core;

import boza.rhi;
import boza.rhi.render_context;

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
        if (!rhi::RenderContext::initialized())
        {
            Log::error("Render context not initialized.");
            return;
        }

        auto rhi_sampler = create_sampler(
            rhi::RenderContext::api(), {
                .device = rhi::RenderContext::device(),
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

        if (!rhi_sampler)
        {
            Log::error("Failed to create sampler: {}", name_);
            return;
        }

        rhi_sampler_.reset(rhi_sampler.release());
    }

    Sampler::~Sampler() = default;

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
          rhi_sampler_{ std::move(other.rhi_sampler_) } {}

    Sampler& Sampler::operator=(Sampler&& other) noexcept
    {
        if (this == &other) return *this;

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
        rhi_sampler_    = std::move(other.rhi_sampler_);

        return *this;
    }

    void Sampler::destroy_rhi_sampler(void* handle)
    {
        delete static_cast<rhi::Sampler*>(handle);
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
