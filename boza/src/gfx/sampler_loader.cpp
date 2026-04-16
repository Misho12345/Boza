module boza.gfx.sampler_loader;

import boza.gfx;
import boza.detail;
import boza.core;

namespace boza::gfx
{
    SamplerLoader& SamplerLoader::instance()
    {
        static SamplerLoader instance;
        return instance;
    }

    SamplerLoader::~SamplerLoader() { shutdown(); }

    void SamplerLoader::initialize()
    {
        if (initialized_) return;

        auto [sampler, inserted] = samplers_.try_emplace(
            "boza_default_sampler",
            Sampler{
                "boza_default_sampler",
                SamplerFilter::Linear,
                SamplerWrap::Repeat,
                SamplerWrap::Repeat,
                SamplerWrap::Repeat,
                SamplerFilter::Linear,
                0.0f,
                0.0f,
                1000.0f,
                1.0f
            });

        if (!sampler || !sampler->rhi_handle())
        {
            Log::error("Failed to create default sampler");
            if (inserted) samplers_.erase("boza_default_sampler");
            initialized_ = false;
            return;
        }

        initialized_ = true;
    }

    void SamplerLoader::shutdown()
    {
        if (!initialized_) return;

        samplers_.clear();
        initialized_ = false;
    }

    bool SamplerLoader::load_and_create_samplers()
    {
        const auto sampler_files = detail::AssetPaths::all_sampler_files();

        if (sampler_files.empty())
        {
            Log::info("No sampler files found in {}", detail::AssetPaths::samplers_dir().string());
            return true;
        }

        for (const auto& path : sampler_files)
        {
            auto def_opt = load_sampler_definition(path);
            if (def_opt.has_value()) create(def_opt.value());
        }

        return true;
    }

    std::optional<SamplerDefinition> SamplerLoader::load_sampler_definition(const fs::path& path)
    {
        const auto json_opt = detail::FileIO::load_json(path);
        if (!json_opt.has_value())
        {
            Log::error("Failed to load sampler file: {}", path.string());
            return std::nullopt;
        }

        const auto& j = json_opt.value();
        SamplerDefinition def;

        def.name = detail::AssetPaths::sampler_id_from_path(path);
        if (def.name.empty())
        {
            Log::error("Failed to derive sampler id from path: {}", path.string());
            return std::nullopt;
        }

        auto parse_filter = [](const std::string& str) -> SamplerFilter
        {
            if (str == "nearest") return SamplerFilter::Nearest;
            if (str == "linear") return SamplerFilter::Linear;
            if (str == "anisotropic") return SamplerFilter::Anisotropic;
            return SamplerFilter::Linear;
        };

        auto parse_wrap = [](const std::string& str) -> SamplerWrap
        {
            if (str == "repeat") return SamplerWrap::Repeat;
            if (str == "clamp_to_edge") return SamplerWrap::ClampToEdge;
            if (str == "clamp_to_border") return SamplerWrap::ClampToBorder;
            if (str == "mirror") return SamplerWrap::Mirror;
            return SamplerWrap::Repeat;
        };

        if (j.contains("filter") && j["filter"].is_string())
            def.filter = parse_filter(j["filter"].get<std::string>());
        if (j.contains("wrap_u") && j["wrap_u"].is_string())
            def.wrap_u = parse_wrap(j["wrap_u"].get<std::string>());
        if (j.contains("wrap_v") && j["wrap_v"].is_string())
            def.wrap_v = parse_wrap(j["wrap_v"].get<std::string>());
        if (j.contains("wrap_w") && j["wrap_w"].is_string())
            def.wrap_w = parse_wrap(j["wrap_w"].get<std::string>());

        if (j.contains("mipmap_mode") && j["mipmap_mode"].is_string())
            def.mipmap_mode = parse_filter(j["mipmap_mode"].get<std::string>());
        if (j.contains("mip_lod_bias") && j["mip_lod_bias"].is_number())
            def.mip_lod_bias = j["mip_lod_bias"].get<float>();

        if (j.contains("min_lod") && j["min_lod"].is_number())
            def.min_lod = j["min_lod"].get<float>();
        if (j.contains("max_lod") && j["max_lod"].is_number())
            def.max_lod = j["max_lod"].get<float>();

        if (j.contains("max_anisotropy") && j["max_anisotropy"].is_number())
            def.max_anisotropy = j["max_anisotropy"].get<float>();

        return def;
    }

    Sampler& SamplerLoader::create(
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
        const std::string name_str{ name };

        if (auto* existing = samplers_.find_ptr(name_str))
        {
            Log::warn("Sampler '{}' already exists, returning existing sampler", name_str);
            return *existing;
        }

        Sampler sampler{
            name,
            filter,
            wrap_u,
            wrap_v,
            wrap_w,
            mipmap_mode,
            mip_lod_bias,
            min_lod,
            max_lod,
            max_anisotropy
        };

        auto [created_sampler, inserted] = samplers_.try_emplace(name_str, std::move(sampler));

        if (!created_sampler || !created_sampler->rhi_handle())
        {
            Log::error("Failed to create sampler: {}", name_str);
            if (inserted) samplers_.erase(name_str);
            return default_sampler();
        }

        // Log::trace("Created sampler: {}", name_str);
        return *created_sampler;
    }

    Sampler& SamplerLoader::create(const SamplerDefinition& def)
    {
        return create(
            def.name,
            def.filter,
            def.wrap_u,
            def.wrap_v,
            def.wrap_w,
            def.mipmap_mode,
            def.mip_lod_bias,
            def.min_lod,
            def.max_lod,
            def.max_anisotropy);
    }

    Sampler& SamplerLoader::get_sampler(const std::string_view name)
    {
        if (auto* sampler = try_get_sampler(name)) return *sampler;
        return default_sampler();
    }

    Sampler* SamplerLoader::try_get_sampler(const std::string_view name)
    {
        return samplers_.find_ptr(name);
    }

    void SamplerLoader::destroy(const std::string_view name)
    {
        const std::string name_str{ name };

        if (name_str == "boza_default_sampler")
        {
            Log::warn("Cannot destroy default sampler");
            return;
        }

        if (samplers_.erase(name_str))
        {
            Log::trace("Destroyed sampler: {}", name_str);
        }
        else Log::warn("Attempted to destroy non-existent sampler: {}", name_str);
    }

    Sampler& SamplerLoader::default_sampler()
    {
        auto* sampler = samplers_.find_ptr("boza_default_sampler");
        assert(sampler != nullptr, "Default sampler not initialized");
        return *sampler;
    }
}
