module boza.gfx.sampler_loader;

import boza.detail;

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

        default_sampler_ = Sampler::create_internal(
            "boza_default_sampler",
            SamplerFilter::Linear,
            SamplerWrap::Repeat,
            SamplerWrap::Repeat,
            SamplerWrap::Repeat,
            SamplerFilter::Linear,
            0.0f, 0.0f, 1000.0f, 1.0f);

        if (default_sampler_)
        {
            owned_samplers_.insert(default_sampler_);
            // Log::trace("Created default sampler (linear/repeat)");
        }
        else Log::error("Failed to create default sampler");

        initialized_ = true;
    }

    void SamplerLoader::shutdown()
    {
        if (!initialized_) return;

        samplers_.clear();

        for (const auto* sampler : owned_samplers_)
        {
            if (sampler) delete sampler;
        }

        owned_samplers_.clear();

        default_sampler_ = nullptr;
        definitions_.clear();
        initialized_ = false;
    }

    bool SamplerLoader::load_all_sampler_definitions()
    {
        const auto samplers_dir = detail::AssetPaths::samplers_dir();

        if (!exists(samplers_dir))
        {
            Log::warn("Samplers directory not found: {}", samplers_dir.string());
            return true;
        }

        std::vector<fs::path> sampler_files;
        for (const auto& entry : fs::directory_iterator(samplers_dir))
        {
            if (entry.is_regular_file() &&
                entry.path().extension() == ".json" &&
                entry.path().stem().extension() == ".smpl")
            {
                sampler_files.push_back(entry.path());
            }
        }

        if (sampler_files.empty())
        {
            Log::info("No sampler files found in {}", samplers_dir.string());
            return true;
        }

        for (const auto& path : sampler_files)
        {
            auto def_opt = load_sampler_definition(path);
            if (def_opt.has_value())
            {
                const auto& def = def_opt.value();
                definitions_[def.name] = def;
                // Log::trace("Loaded sampler definition: {}", def.name);
            }
        }

        // Log::trace("Loaded {} sampler definitions", definitions_.size());
        return true;
    }

    bool SamplerLoader::create_all_samplers()
    {
        for (const auto& [name, def] : definitions_)
        {
            auto* sampler = create_sampler_from_definition(def);
            if (sampler)
            {
                samplers_[name] = sampler;
                owned_samplers_.insert(sampler);
                // Log::trace("Created sampler: {}", name);
            }
            else Log::error("Failed to create sampler: {}", name);
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

        const std::string filename = path.stem().string();
        def.name = filename.ends_with(".smpl") ? filename.substr(0, filename.size() - 5) : filename;

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

    Sampler* SamplerLoader::create_sampler_from_definition(const SamplerDefinition& def)
    {
        return Sampler::create_internal(
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

    Sampler* SamplerLoader::get_or_load(const std::string& name)
    {
        if (samplers_.contains(name)) return samplers_[name];

        if (definitions_.contains(name))
        {
            const auto& def = definitions_[name];
            auto* sampler = create_sampler_from_definition(def);
            if (sampler)
            {
                samplers_[name] = sampler;
                owned_samplers_.insert(sampler);
                Log::trace("Created on-demand sampler: {}", name);
                return sampler;
            }
        }

        Log::warn("Sampler '{}' not found, returning default sampler", name);
        return default_sampler_;
    }

    Sampler* SamplerLoader::get_sampler(const std::string& name) const
    {
        const auto it = samplers_.find(name);
        return it != samplers_.end() ? it->second : nullptr;
    }

    void SamplerLoader::register_sampler(const std::string& name, Sampler* sampler, const bool take_ownership)
    {
        if (!sampler)
        {
            Log::warn("Cannot register null sampler '{}'", name);
            return;
        }

        if (samplers_.contains(name))
        {
            auto* existing = samplers_[name];
            if (existing != sampler && owned_samplers_.contains(existing))
            {
                Log::warn("Sampler '{}' already registered, replacing and deleting old sampler", name);
                owned_samplers_.erase(existing);
                delete existing;
            }
        }

        samplers_[name] = sampler;

        if (take_ownership)
            owned_samplers_.insert(sampler);

        // Log::trace("Registered sampler: {} (owned: {})", name, take_ownership);
    }

    void SamplerLoader::unregister_sampler(Sampler* sampler)
    {
        if (!sampler) return;

        for (auto it = samplers_.begin(); it != samplers_.end();)
        {
            if (it->second == sampler)
            {
                Log::trace("Unregistered sampler: {}", it->first);
                it = samplers_.erase(it);
            }
            else ++it;
        }

        owned_samplers_.erase(sampler);
    }
}