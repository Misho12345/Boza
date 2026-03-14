module;

#include "api.hpp"

export module boza.gfx:sampler;

import std;
import boza.common;

namespace boza::gfx
{
    class SamplerLoader;
}

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
        [[nodiscard]] SamplerFilter get_filter() const { return filter_; }

        [[nodiscard]] SamplerWrap get_wrap_u() const { return wrap_u_; }
        [[nodiscard]] SamplerWrap get_wrap_v() const { return wrap_v_; }
        [[nodiscard]] SamplerWrap get_wrap_w() const { return wrap_w_; }

        [[nodiscard]] SamplerFilter get_mipmap_mode() const { return mipmap_mode_; }

        [[nodiscard]] float get_mip_lod_bias() const { return mip_lod_bias_; }
        [[nodiscard]] float get_min_lod() const { return min_lod_; }
        [[nodiscard]] float get_max_lod() const { return max_lod_; }
        [[nodiscard]] float get_max_anisotropy() const { return max_anisotropy_; }

    public:
        ~Sampler();

        Sampler(const Sampler&)            = delete;
        Sampler& operator=(const Sampler&) = delete;
        Sampler(Sampler&&) noexcept;
        Sampler& operator=(Sampler&&) noexcept;

        static Sampler& create(
            std::string_view name,
            SamplerFilter    filter         = SamplerFilter::Linear,
            SamplerWrap      wrap_u         = SamplerWrap::Repeat,
            SamplerWrap      wrap_v         = SamplerWrap::Repeat,
            SamplerWrap      wrap_w         = SamplerWrap::Repeat,
            SamplerFilter    mipmap_mode    = SamplerFilter::Linear,
            float            mip_lod_bias   = 0.0f,
            float            min_lod        = 0.0f,
            float            max_lod        = 1000.0f,
            float            max_anisotropy = 1.0f);

        static Sampler& get(std::string_view name);
        static Sampler* try_get(std::string_view name);

        static void destroy(std::string_view name);

        [[msvc::no_unique_address]] Property<Sampler, &Sampler::get_filter>         filter{ this };
        [[msvc::no_unique_address]] Property<Sampler, &Sampler::get_wrap_u>         wrap_u{ this };
        [[msvc::no_unique_address]] Property<Sampler, &Sampler::get_wrap_v>         wrap_v{ this };
        [[msvc::no_unique_address]] Property<Sampler, &Sampler::get_wrap_w>         wrap_w{ this };
        [[msvc::no_unique_address]] Property<Sampler, &Sampler::get_mipmap_mode>    mipmap_mode{ this };
        [[msvc::no_unique_address]] Property<Sampler, &Sampler::get_mip_lod_bias>   mip_lod_bias{ this };
        [[msvc::no_unique_address]] Property<Sampler, &Sampler::get_min_lod>        min_lod{ this };
        [[msvc::no_unique_address]] Property<Sampler, &Sampler::get_max_lod>        max_lod{ this };
        [[msvc::no_unique_address]] Property<Sampler, &Sampler::get_max_anisotropy> max_anisotropy{ this };

        [[nodiscard]] std::string_view name() const { return name_; }

    private:
        using RhiSamplerHandle = std::unique_ptr<void, void(*)(void*)>;

        [[nodiscard]] void* rhi_handle() const { return rhi_sampler_.get(); }

        static void destroy_rhi_sampler(void* handle);

        Sampler(
            std::string_view name,
            SamplerFilter    filter,
            SamplerWrap      wrap_u,
            SamplerWrap      wrap_v,
            SamplerWrap      wrap_w,
            SamplerFilter    mipmap_mode,
            float            mip_lod_bias,
            float            min_lod,
            float            max_lod,
            float            max_anisotropy);

        std::string name_;
        RhiSamplerHandle rhi_sampler_{ nullptr, &Sampler::destroy_rhi_sampler };

        SamplerFilter filter_;
        SamplerWrap   wrap_u_;
        SamplerWrap   wrap_v_;
        SamplerWrap   wrap_w_;

        SamplerFilter mipmap_mode_;

        float mip_lod_bias_;
        float min_lod_;
        float max_lod_;
        float max_anisotropy_;

        friend class Material;
        friend class gfx::SamplerLoader;
    };
}
