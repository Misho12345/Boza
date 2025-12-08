module boza.app;

import :material_loader;

import std;
import boza.common;
import boza.core;
import boza.detail;
import boza.gfx;
import boza.rhi;

namespace boza::app
{
    using detail::AssetPaths;
    using detail::FileIO;

    struct CameraUBO
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct LightUBO
    {
        glm::vec4 light_position;
        glm::vec4 light_color;
        glm::vec4 view_pos;
    };

    struct TimeUBO
    {
        float time;
        float delta_time;
        float padding[2];
    };

    MaterialLoader::~MaterialLoader()
    {
        shutdown();
    }

    bool MaterialLoader::initialize(
        rhi::Device*         device,
        rhi::Swapchain*      swapchain,
        rhi::DescriptorPool* descriptor_pool,
        rhi::ResourceCache*  resource_cache,
        const rhi::GraphicsApi     api)
    {
        if (initialized_) return true;

        device_          = device;
        swapchain_       = swapchain;
        descriptor_pool_ = descriptor_pool;
        resource_cache_  = resource_cache;
        api_             = api;

        camera_ubo_.reset(rhi::create_buffer(
            api_, {
                .device = device_,
                .size = sizeof(CameraUBO),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible,
                .access_mode = rhi::ResourceAccessMode::Dynamic
            }));

        light_ubo_.reset(rhi::create_buffer(
            api_, {
                .device = device_,
                .size = sizeof(LightUBO),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible,
                .access_mode = rhi::ResourceAccessMode::Dynamic
            }));

        const LightUBO light_data{
            .light_position = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f),
            .light_color = glm::vec4(1.0f, 1.0f, 1.0f, 2.0f),
            .view_pos = glm::vec4(0.0f, 0.0f, 5.0f, 1.0f)
        };
        light_ubo_->upload(&light_data, sizeof(LightUBO), 0);

        time_ubo_.reset(rhi::create_buffer(
            api_, {
                .device = device_,
                .size = sizeof(TimeUBO),
                .usage = rhi::BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible,
                .access_mode = rhi::ResourceAccessMode::Dynamic
            }));

        default_sampler_.reset(rhi::create_sampler(
            api_, {
                .device = device_,
                .filter = rhi::SamplerFilter::Linear,
                .address_mode_u = rhi::SamplerAddressMode::Repeat,
                .address_mode_v = rhi::SamplerAddressMode::Repeat,
                .address_mode_w = rhi::SamplerAddressMode::Repeat
            }));

        error_texture_ = new Texture(
            1, 1,
            TextureFormat::RGBA8,
            static_cast<std::uint32_t>(TextureUsage::Sampled) |
            static_cast<std::uint32_t>(TextureUsage::TransferDst),
            TextureAccessMode::Static);

        if (error_texture_)
        {
            const std::array<std::uint8_t, 4> magenta{ 255, 0, 255, 255 };
            error_texture_->upload(magenta.data(), magenta.size());
            Log::trace("Created error texture (1x1 magenta)");
        }

        initialized_ = true;
        return true;
    }

    void MaterialLoader::shutdown()
    {
        if (!initialized_) return;

        initialized_ = false;

        for (auto& material : materials_ | std::views::values)
        {
            if (material)
            {
                delete material;
                material = nullptr;
            }
        }
        materials_.clear();

        for (auto& texture : loaded_textures_ | std::views::values)
        {
            if (texture)
            {
                delete texture;
                texture = nullptr;
            }
        }
        loaded_textures_.clear();

        if (error_texture_)
        {
            delete error_texture_;
            error_texture_ = nullptr;
        }

        default_sampler_.reset();
        time_ubo_.reset();
        light_ubo_.reset();
        camera_ubo_.reset();

        definitions_.clear();
        initialized_ = false;
    }

    bool MaterialLoader::load_all_material_definitions()
    {
        const auto material_files = AssetPaths::all_material_files();

        if (material_files.empty())
        {
            Log::warn("No material files found in {}", AssetPaths::materials_dir().string());
            return true;
        }

        for (const auto& path : material_files)
        {
            auto def_opt = load_material_definition(path);
            if (def_opt.has_value())
            {
                const auto& def = def_opt.value();
                definitions_[def.name] = def;
                Log::trace("Loaded material definition: {} ({})",
                    def.name,
                    def.load_strategy == LoadStrategy::GameLoad ? "game_load" : "on_demand");
            }
        }

        Log::info("Loaded {} material definitions", definitions_.size());
        return true;
    }

    bool MaterialLoader::create_game_load_materials()
    {
        for (const auto& [name, def] : definitions_)
        {
            if (def.load_strategy == LoadStrategy::GameLoad)
            {
                auto* material = create_material_from_definition(def);
                if (material)
                {
                    materials_[name] = material;
                    bind_engine_resources(material);
                    setup_material_from_definition(material, def);
                    Log::trace("Created material: {}", name);
                }
                else
                {
                    Log::error("Failed to create material: {}", name);
                }
            }
        }
        return true;
    }

    std::optional<MaterialDefinition> MaterialLoader::load_material_definition(const fs::path& path)
    {
        auto json_opt = FileIO::load_json(path);
        if (!json_opt.has_value())
        {
            Log::error("Failed to load material file: {}", path.string());
            return std::nullopt;
        }

        const auto& j = json_opt.value();

        MaterialDefinition def;

        const std::string filename = path.stem().string();
        if (filename.ends_with(".mat"))
        {
            def.name = filename.substr(0, filename.size() - 4);
        }
        else
        {
            def.name = filename;
        }

        if (!j.contains("vertex_shader") || !j["vertex_shader"].is_string())
        {
            Log::error("Material {} missing 'vertex_shader' string", def.name);
            return std::nullopt;
        }
        def.vertex_shader = j["vertex_shader"].get<std::string>();

        if (!j.contains("fragment_shader") || !j["fragment_shader"].is_string())
        {
            Log::error("Material {} missing 'fragment_shader' string", def.name);
            return std::nullopt;
        }
        def.fragment_shader = j["fragment_shader"].get<std::string>();

        if (j.contains("load_strategy") && j["load_strategy"].is_string())
        {
            const std::string strategy = j["load_strategy"].get<std::string>();
            if (strategy == "game_load")
            {
                def.load_strategy = LoadStrategy::GameLoad;
            }
            else if (strategy == "on_demand")
            {
                def.load_strategy = LoadStrategy::OnDemand;
            }
            else
            {
                Log::warn("Material {} has unknown load_strategy '{}', defaulting to game_load",
                    def.name, strategy);
            }
        }

        auto parse_texture_format = [](const std::string& format_str) -> TextureFormat
        {
            if (format_str == "R8") return TextureFormat::R8;
            if (format_str == "RG8") return TextureFormat::RG8;
            if (format_str == "RGB8") return TextureFormat::RGB8;
            if (format_str == "RGBA8") return TextureFormat::RGBA8;
            if (format_str == "BGRA8") return TextureFormat::BGRA8;
            if (format_str == "R16F") return TextureFormat::R16F;
            if (format_str == "RG16F") return TextureFormat::RG16F;
            if (format_str == "RGB16F") return TextureFormat::RGB16F;
            if (format_str == "RGBA16F") return TextureFormat::RGBA16F;
            if (format_str == "R32F") return TextureFormat::R32F;
            if (format_str == "RG32F") return TextureFormat::RG32F;
            if (format_str == "RGB32F") return TextureFormat::RGB32F;
            if (format_str == "RGBA32F") return TextureFormat::RGBA32F;
            return TextureFormat::RGBA8;
        };

        if (j.contains("textures") && j["textures"].is_object())
        {
            for (const auto& [key, val] : j["textures"].items())
            {
                if (val.is_string())
                {
                    def.textures[key] = TextureInfo{ .file = val.get<std::string>() };
                }
                else if (val.is_object())
                {
                    TextureInfo info;
                    if (val.contains("file") && val["file"].is_string())
                    {
                        info.file = val["file"].get<std::string>();
                    }
                    if (val.contains("format") && val["format"].is_string())
                    {
                        info.format = parse_texture_format(val["format"].get<std::string>());
                    }
                    def.textures[key] = info;
                }
            }
        }

        if (j.contains("properties") && j["properties"].is_object())
        {
            for (const auto& [key, val] : j["properties"].items())
            {
                if (val.is_array() && val.size() == 4)
                {
                    glm::vec4 v;
                    for (int i = 0; i < 4; ++i)
                    {
                        v[i] = val[i].is_number() ? val[i].get<float>() : 0.0f;
                    }
                    def.vec4_properties[key] = v;
                }
            }
        }

        return def;
    }

    Material* MaterialLoader::create_material_from_definition(const MaterialDefinition& def)
    {
        return Material::create(def.vertex_shader, def.fragment_shader);
    }

    void MaterialLoader::bind_engine_resources(Material* material)
    {
        if (material->descriptor_set_count() == 0) return;

        auto* desc_set = static_cast<rhi::DescriptorSet*>(material->rhi_descriptor_set_handle(0));
        if (!desc_set) return;

        std::vector<rhi::DescriptorWrite> writes;

        auto try_bind_ubo = [&](const std::string& ubo_name, rhi::Buffer* buffer, std::size_t size)
        { const auto info = material->lookup_binding(ubo_name);
            if (info.has_value() && info->descriptor_type == static_cast<std::uint32_t>(rhi::DescriptorType::UniformBuffer))
            {
                writes.push_back({
                    .binding = info->binding,
                    .array_element = 0,
                    .type = rhi::DescriptorType::UniformBuffer,
                    .info = rhi::UniformBuffer{
                        .buffer = buffer,
                        .offset = 0,
                        .range = static_cast<std::uint32_t>(size)
                    }
                });
                return true;
            }
            return false;
        };

        try_bind_ubo("cam", camera_ubo_.get(), sizeof(CameraUBO));
        try_bind_ubo("lightUBO", light_ubo_.get(), sizeof(LightUBO));
        try_bind_ubo("timeData", time_ubo_.get(), sizeof(TimeUBO));

        auto try_bind_sampler = [&](const std::string& sampler_name, const Texture* texture)
        {
            const auto info = material->lookup_binding(sampler_name);
            if (info.has_value() && info->descriptor_type == static_cast<std::uint32_t>(rhi::DescriptorType::CombinedImageSampler))
            {
                writes.push_back({
                    .binding = info->binding,
                    .array_element = 0,
                    .type = rhi::DescriptorType::CombinedImageSampler,
                    .info = rhi::CombinedImageSampler{
                        .sampler = texture->rhi_sampler_handle()
                            ? static_cast<rhi::Sampler*>(texture->rhi_sampler_handle())
                            : default_sampler_.get(),
                        .texture = static_cast<rhi::Texture*>(texture->rhi_handle())
                    }
                });
                return true;
            }
            return false;
        };

        try_bind_sampler("albedo_map", error_texture_);

        if (!writes.empty())
        {
            desc_set->update(writes);
        }
    }

    void MaterialLoader::setup_material_from_definition(Material* material, const MaterialDefinition& def)
    {
        for (const auto& [prop_name, tex_info] : def.textures)
        {
            Texture* texture = error_texture_;

            const std::string cache_key = tex_info.file + "_" + std::to_string(static_cast<int>(tex_info.format));

            if (!loaded_textures_.contains(cache_key))
            {
                const fs::path tex_path = AssetPaths::resolve_texture(tex_info.file);
                auto* loaded = Texture::load_from_file(tex_path.string(), tex_info.format, TextureAccessMode::Static);
                if (loaded)
                {
                    loaded_textures_[cache_key] = loaded;
                    Log::trace("Loaded texture: {} (format: {})", tex_info.file, static_cast<int>(tex_info.format));
                }
                else
                {
                    Log::warn("Failed to load texture: {}, using error texture", tex_info.file);
                }
            }

            if (loaded_textures_.contains(cache_key))
            {
                texture = loaded_textures_[cache_key];
            }

            material->update_texture(prop_name, texture);
        }

        for (const auto& [prop_name, value] : def.vec4_properties)
        {
            material->update_property(prop_name, value);
        }
    }

    Material* MaterialLoader::get_or_create_material(const std::string& name)
    {
        if (materials_.contains(name))
        {
            return materials_[name];
        }

        if (definitions_.contains(name))
        {
            const auto& def = definitions_[name];
            auto* material = create_material_from_definition(def);
            if (material)
            {
                materials_[name] = material;
                bind_engine_resources(material);
                setup_material_from_definition(material, def);
                Log::trace("Created on-demand material: {}", name);
                return material;
            }
        }

        Log::warn("Material '{}' not found", name);
        return nullptr;
    }

    Material* MaterialLoader::get_material(const std::string& name) const
    {
        const auto it = materials_.find(name);
        if (it != materials_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void MaterialLoader::register_material(const std::string& name, Material* material)
    {
        if (!material)
        {
            Log::warn("Cannot register null material '{}'", name);
            return;
        }

        if (materials_.contains(name))
        {
            Log::warn("Material '{}' already registered, replacing", name);
            delete materials_[name];
        }

        materials_[name] = material;
        bind_engine_resources(material);

        Log::trace("Registered custom material: {}", name);
    }

    void MaterialLoader::update_time_ubo(float time, float delta_time)
    {
        if (!time_ubo_) return;

        const TimeUBO time_data{
            .time = time,
            .delta_time = delta_time,
            .padding = { 0.0f, 0.0f }
        };
        time_ubo_->upload(&time_data, sizeof(TimeUBO), 0);
    }
}

