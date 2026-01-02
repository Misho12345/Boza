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

        bool load_all_sampler_definitions();
        bool create_all_samplers();

        Sampler* get_or_load(const std::string& name);
        Sampler* get_sampler(const std::string& name) const;

        void register_sampler(const std::string& name, Sampler* sampler, bool take_ownership = true);
        void unregister_sampler(Sampler* sampler);

        [[nodiscard]] Sampler* default_sampler() const { return default_sampler_; }
        [[nodiscard]] bool initialized() const { return initialized_; }

    private:
        SamplerLoader() = default;
        ~SamplerLoader();

        static std::optional<SamplerDefinition> load_sampler_definition(const std::filesystem::path& path);
        static Sampler* create_sampler_from_definition(const SamplerDefinition& def);

        flat_map<std::string, SamplerDefinition> definitions_;
        flat_map<std::string, Sampler*> samplers_;
        std::unordered_set<Sampler*> owned_samplers_;
        Sampler* default_sampler_{ nullptr };
        bool initialized_{ false };
    };
}