export module boza.gfx.sampler_loader;

import std;
import boza.common;
import boza.core;
import boza.gfx;

export namespace boza::gfx
{
    struct SamplerDefinition
    {
        std::string   name;
        SamplerFilter filter{ SamplerFilter::Linear };
        SamplerWrap   wrap_u{ SamplerWrap::Repeat };
        SamplerWrap   wrap_v{ SamplerWrap::Repeat };
        SamplerWrap   wrap_w{ SamplerWrap::Repeat };
        SamplerFilter mipmap_mode{ SamplerFilter::Linear };
        float         mip_lod_bias{ 0.0f };
        float         min_lod{ 0.0f };
        float         max_lod{ 1000.0f };
        float         max_anisotropy{ 1.0f };
    };

    class SamplerLoader final
    {
    public:
        static SamplerLoader& instance();

        SamplerLoader(const SamplerLoader&) = delete;
        SamplerLoader& operator=(const SamplerLoader&) = delete;
        SamplerLoader(SamplerLoader&&) = delete;
        SamplerLoader& operator=(SamplerLoader&&) = delete;

        void initialize();
        void shutdown();

        bool load_and_create_samplers();

        Sampler& get_sampler(std::string_view name);
        Sampler* try_get_sampler(std::string_view name);

        [[nodiscard]] Sampler& default_sampler();
        [[nodiscard]] bool initialized() const { return initialized_; }

    private:
        SamplerLoader() = default;
        ~SamplerLoader();

        static std::optional<SamplerDefinition> load_sampler_definition(const fs::path& path);

        Sampler& create(
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

        Sampler& create(const SamplerDefinition& def);
        void destroy(std::string_view name);

        node_map<std::string, Sampler> samplers_;
        bool initialized_{ false };

        friend class Sampler;
    };
}