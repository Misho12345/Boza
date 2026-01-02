module;

#include "api.hpp"
#include <cstddef>

export module boza.gfx:sampler;

import boza.common;

export namespace boza
{
    enum class SamplerFilter : std::uint8_t
    {
        Nearest,
        Linear,
        Anisotropic
    };

    enum class SamplerWrap : std::uint8_t
    {
        Repeat,
        ClampToEdge,
        ClampToBorder,
        Mirror
    };

    class BOZA_API Sampler final
    {
    public:
        ~Sampler();

        Sampler(const Sampler&)            = delete;
        Sampler& operator=(const Sampler&) = delete;
        Sampler(Sampler&&)                 = delete;
        Sampler& operator=(Sampler&&)      = delete;

        static Sampler* create(
            const std::string& name,
            SamplerFilter      filter = SamplerFilter::Linear,
            SamplerWrap        wrap   = SamplerWrap::Repeat);

        static Sampler* create_internal(
            const std::string& name,
            SamplerFilter      filter,
            SamplerWrap        wrap_u,
            SamplerWrap        wrap_v,
            SamplerWrap        wrap_w,
            SamplerFilter      mipmap_mode,
            float              mip_lod_bias,
            float              min_lod,
            float              max_lod,
            float              max_anisotropy);

        static Sampler* get(const std::string& name);
        static void     destroy(Sampler* sampler);

        PropertyGet<Sampler, SamplerFilter> filter{ &Sampler::get_filter, offsetof(Sampler, filter_) };
        PropertyGet<Sampler, SamplerWrap> wrap_u{ &Sampler::get_wrap_u, offsetof(Sampler, wrap_u_) };
        PropertyGet<Sampler, SamplerWrap> wrap_v{ &Sampler::get_wrap_v, offsetof(Sampler, wrap_v_) };
        PropertyGet<Sampler, SamplerWrap> wrap_w{ &Sampler::get_wrap_w, offsetof(Sampler, wrap_w_) };
        PropertyGet<Sampler, SamplerFilter> mipmap_mode{ &Sampler::get_mipmap_mode, offsetof(Sampler, mipmap_mode_) };
        PropertyGet<Sampler, float> mip_lod_bias{ &Sampler::get_mip_lod_bias, offsetof(Sampler, mip_lod_bias_) };
        PropertyGet<Sampler, float> min_lod{ &Sampler::get_min_lod, offsetof(Sampler, min_lod_) };
        PropertyGet<Sampler, float> max_lod{ &Sampler::get_max_lod, offsetof(Sampler, max_lod_) };
        PropertyGet<Sampler, float> max_anisotropy{ &Sampler::get_max_anisotropy, offsetof(Sampler, max_anisotropy_) };

        [[nodiscard]] void* rhi_handle() const { return rhi_sampler_; }

    private:
        Sampler(
            SamplerFilter filter,
            SamplerWrap   wrap_u,
            SamplerWrap   wrap_v,
            SamplerWrap   wrap_w,
            SamplerFilter mipmap_mode,
            float         mip_lod_bias,
            float         min_lod,
            float         max_lod,
            float         max_anisotropy);

        [[nodiscard]] SamplerFilter get_filter() const { return filter_; }
        [[nodiscard]] SamplerWrap   get_wrap_u() const { return wrap_u_; }
        [[nodiscard]] SamplerWrap   get_wrap_v() const { return wrap_v_; }
        [[nodiscard]] SamplerWrap   get_wrap_w() const { return wrap_w_; }
        [[nodiscard]] SamplerFilter get_mipmap_mode() const { return mipmap_mode_; }
        [[nodiscard]] float         get_mip_lod_bias() const { return mip_lod_bias_; }
        [[nodiscard]] float         get_min_lod() const { return min_lod_; }
        [[nodiscard]] float         get_max_lod() const { return max_lod_; }
        [[nodiscard]] float         get_max_anisotropy() const { return max_anisotropy_; }

        void* rhi_sampler_{ nullptr };

        SamplerFilter filter_;
        SamplerWrap   wrap_u_;
        SamplerWrap   wrap_v_;
        SamplerWrap   wrap_w_;

        SamplerFilter mipmap_mode_;

        float mip_lod_bias_;
        float min_lod_;
        float max_lod_;
        float max_anisotropy_;
    };
}
