module boza.gfx.material_loader;

import boza.gfx.texture_loader;
import boza.gfx.sampler_loader;

namespace boza::gfx
{
    MaterialLoader& MaterialLoader::instance()
    {
        static MaterialLoader instance;
        return instance;
    }

    MaterialLoader::~MaterialLoader() { shutdown(); }

    bool MaterialLoader::initialize(
        rhi::Device*           device,
        rhi::Swapchain*        swapchain,
        rhi::DescriptorPool*   descriptor_pool,
        rhi::ResourceCache*    resource_cache,
        const rhi::GraphicsApi api)
    {
        if (initialized_) return true;

        device_          = device;
        swapchain_       = swapchain;
        descriptor_pool_ = descriptor_pool;
        resource_cache_  = resource_cache;
        api_             = api;

        camera_ubo_.reset(create_buffer(
            api_, {
                .device = device_,
                .size = sizeof(CameraUBO),
                .usage = BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

        light_ubo_.reset(create_buffer(
            api_, {
                .device = device_,
                .size = sizeof(LightUBO),
                .usage = BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

        static constexpr LightUBO light_data{
            .light_position = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f),
            .light_color = glm::vec4(1.0f, 1.0f, 1.0f, 2.0f),
            .view_pos = glm::vec4(0.0f, 0.0f, 5.0f, 1.0f)
        };
        light_ubo_->upload(&light_data, sizeof(LightUBO), 0);

        time_ubo_.reset(create_buffer(
            api_, {
                .device = device_,
                .size = sizeof(TimeUBO),
                .usage = BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible
            }));

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

        time_ubo_.reset();
        light_ubo_.reset();
        camera_ubo_.reset();

        definitions_.clear();
        initialized_ = false;
    }

    bool MaterialLoader::load_all_material_definitions()
    {
        const auto material_files = detail::AssetPaths::all_material_files();

        if (material_files.empty())
        {
            Log::warn("No material files found in {}", detail::AssetPaths::materials_dir().string());
            return true;
        }

        for (const auto& path : material_files)
        {
            auto def_opt = load_material_definition(path);
            if (def_opt.has_value())
            {
                const auto& def        = def_opt.value();
                definitions_[def.name] = def;
                // Log::trace("Loaded material definition: {} ({})",
                //            def.name,
                //            def.load_strategy == LoadStrategy::GameLoad ? "game_load" : "on_demand");
            }
        }

        // Log::trace("Loaded {} material definitions", definitions_.size());
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
                    // Log::trace("Created material: {}", name);
                }
                else Log::error("Failed to create material: {}", name);
            }
        }
        return true;
    }

    std::optional<MaterialDefinition> MaterialLoader::load_material_definition(const std::filesystem::path& path)
    {
        auto json_opt = detail::FileIO::load_json(path);
        if (!json_opt.has_value())
        {
            Log::error("Failed to load material file: {}", path.string());
            return std::nullopt;
        }

        const auto& j = json_opt.value();

        MaterialDefinition def;

        const std::string filename = path.stem().string();

        def.name = filename.ends_with(".mat") ? filename.substr(0, filename.size() - 4) : filename;

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
            if (strategy == "game_load") def.load_strategy = LoadStrategy::GameLoad;
            else if (strategy == "on_demand") def.load_strategy = LoadStrategy::OnDemand;
            else Log::warn("Material {} has unknown load_strategy '{}', defaulting to game_load", def.name, strategy);
        }

        if (j.contains("settings") && j["settings"].is_object())
        {
            const auto& s = j["settings"];

            if (s.contains("depth_compare") && s["depth_compare"].is_string())
            {
                const std::string cmp = s["depth_compare"].get<std::string>();
                if (cmp == "never") def.settings.depth_compare_op = CompareOp::Never;
                else if (cmp == "less") def.settings.depth_compare_op = CompareOp::Less;
                else if (cmp == "equal") def.settings.depth_compare_op = CompareOp::Equal;
                else if (cmp == "less_or_equal") def.settings.depth_compare_op = CompareOp::LessOrEqual;
                else if (cmp == "greater") def.settings.depth_compare_op = CompareOp::Greater;
                else if (cmp == "not_equal") def.settings.depth_compare_op = CompareOp::NotEqual;
                else if (cmp == "greater_or_equal") def.settings.depth_compare_op = CompareOp::GreaterOrEqual;
                else if (cmp == "always") def.settings.depth_compare_op = CompareOp::Always;
            }

            if (s.contains("depth_test") && s["depth_test"].is_boolean())
                def.settings.depth_test_enable = s["depth_test"].get<bool>();

            if (s.contains("depth_write") && s["depth_write"].is_boolean())
                def.settings.depth_write_enable = s["depth_write"].get<bool>();

            if (s.contains("cull_mode") && s["cull_mode"].is_string())
            {
                const std::string cull = s["cull_mode"].get<std::string>();
                if (cull == "none") def.settings.cull_mode = CullMode::None;
                else if (cull == "front") def.settings.cull_mode = CullMode::Front;
                else if (cull == "back") def.settings.cull_mode = CullMode::Back;
                else if (cull == "front_and_back") def.settings.cull_mode = CullMode::FrontAndBack;
            }

            if (s.contains("front_face") && s["front_face"].is_string())
            {
                const std::string face = s["front_face"].get<std::string>();
                if (face == "ccw" || face == "counter_clockwise") def.settings.front_face = FrontFace::CounterClockwise;
                else if (face == "cw" || face == "clockwise") def.settings.front_face = FrontFace::Clockwise;
            }
        }

        if (j.contains("textures") && j["textures"].is_object())
        {
            for (const auto& [key, val] : j["textures"].items())
            {
                TextureInfo info;

                if (val.is_object())
                {
                    if (val.contains("texture") && val["texture"].is_string()) info.texture_file = val["texture"].get<std::string>();
                    if (val.contains("sampler") && val["sampler"].is_string()) info.sampler_file = val["sampler"].get<std::string>();
                }

                def.textures[key] = info;
            }
        }

        if (j.contains("UBOs") && j["UBOs"].is_object())
        {
            for (const auto& [ubo_name, ubo_obj] : j["UBOs"].items())
            {
                if (!ubo_obj.is_object()) continue;

                flat_map<std::string, PropertyValue> properties;

                for (const auto& [prop_name, prop_val] : ubo_obj.items())
                {
                    if (prop_val.is_boolean())
                    {
                        properties[prop_name] = prop_val.get<bool>();
                    }
                    else if (prop_val.is_number_integer())
                    {
                        if (prop_val.get<std::int64_t>() >= 0)
                            properties[prop_name] = prop_val.get<std::uint32_t>();
                        else
                            properties[prop_name] = prop_val.get<std::int32_t>();
                    }
                    else if (prop_val.is_number_float())
                    {
                        properties[prop_name] = prop_val.get<float>();
                    }
                    else if (prop_val.is_array())
                    {
                        if (prop_val.empty()) continue;

                        if (prop_val[0].is_number_float())
                        {
                            std::vector<float> arr;
                            arr.reserve(prop_val.size());
                            for (const auto& elem : prop_val)
                                arr.push_back(elem.is_number() ? elem.get<float>() : 0.0f);
                            properties[prop_name] = arr;
                        }
                        else if (prop_val[0].is_number_integer())
                        {
                            if (prop_val[0].get<std::int64_t>() >= 0)
                            {
                                std::vector<std::uint32_t> arr;
                                arr.reserve(prop_val.size());
                                for (const auto& elem : prop_val)
                                    arr.push_back(elem.is_number_integer() ? elem.get<std::uint32_t>() : 0u);
                                properties[prop_name] = arr;
                            }
                            else
                            {
                                std::vector<std::int32_t> arr;
                                arr.reserve(prop_val.size());
                                for (const auto& elem : prop_val)
                                    arr.push_back(elem.is_number_integer() ? elem.get<std::int32_t>() : 0);
                                properties[prop_name] = arr;
                            }
                        }
                    }
                }

                def.ubos[ubo_name] = properties;
            }
        }

        return def;
    }

    Material* MaterialLoader::create_material_from_definition(const MaterialDefinition& def)
    {
        return Material::create(def.vertex_shader, def.fragment_shader, def.settings);
    }

    void MaterialLoader::bind_engine_resources(const Material* material) const
    {
        if (material->descriptor_set_count() == 0) return;

        auto* desc_set = static_cast<rhi::DescriptorSet*>(material->rhi_descriptor_set_handle(0));
        if (!desc_set) return;

        std::vector<rhi::DescriptorWrite> writes;

        auto try_bind_ubo = [&](const std::string& ubo_name, rhi::Buffer* buffer, const std::size_t size)
        {
            const auto info = material->lookup_binding(ubo_name);
            if (info.has_value() && info->descriptor_type == static_cast<std::uint32_t>(rhi::DescriptorType::UniformBuffer))
            {
                writes.emplace_back(
                    info->binding,
                    0,
                    rhi::DescriptorType::UniformBuffer,
                    rhi::UniformBuffer{
                        .buffer = buffer,
                        .offset = 0,
                        .range = static_cast<std::uint32_t>(size)
                    }
                );

                return true;
            }

            return false;
        };

        try_bind_ubo("cameraUBO", camera_ubo_.get(), sizeof(CameraUBO));
        try_bind_ubo("lightUBO", light_ubo_.get(), sizeof(LightUBO));
        try_bind_ubo("timeUBO", time_ubo_.get(), sizeof(TimeUBO));

        if (!writes.empty()) desc_set->update(writes);
    }

    void MaterialLoader::setup_material_from_definition(Material* material, const MaterialDefinition& def)
    {
        if (!TextureLoader::instance().initialized()) return;
        if (!SamplerLoader::instance().initialized()) return;

        for (const auto& [prop_name, tex_info] : def.textures)
        {
            const auto binding_info = material->lookup_binding(prop_name);
            if (!binding_info.has_value())
            {
                Log::warn("Texture property '{}' not found in shader for material '{}'", prop_name, def.name);
                continue;
            }

            const auto shader_data_type = static_cast<ShaderDataType>(binding_info->data_type);
            const bool is_cubemap = shader_data_type == ShaderDataType::SamplerCube ||
                                    shader_data_type == ShaderDataType::SamplerCubeArray;

            Texture* texture = is_cubemap
                                   ? TextureLoader::instance().get_or_load_cubemap(tex_info.texture_file)
                                   : TextureLoader::instance().get_or_load(tex_info.texture_file);

            Sampler* sampler = SamplerLoader::instance().get_or_load(tex_info.sampler_file);

            if (texture && sampler) material->update_texture(prop_name, texture, sampler);
        }

        for (const auto& [ubo_name, properties] : def.ubos)
        {
            for (const auto& [prop_name, value] : properties)
            {
                const std::string full_name = ubo_name + "." + prop_name;

                const auto binding_info = material->lookup_binding(full_name);
                if (!binding_info.has_value())
                {
                    Log::warn("UBO property '{}' not found in shader for material '{}'", full_name, def.name);
                    continue;
                }

                std::visit([&]<typename T>(T&& val)
                {
                    using D = std::decay_t<T>;

                    if constexpr (std::is_same_v<D, bool>) material->update_property(full_name, static_cast<std::uint32_t>(val));
                    else if constexpr (std::is_same_v<D, std::int32_t> ||
                        std::is_same_v<D, std::uint32_t> ||
                        std::is_same_v<D, float> ||
                        std::is_same_v<D, double>) material->update_property(full_name, val);
                    else if constexpr (std::is_same_v<D, std::vector<float>>)
                    {
                        if (val.size() == 2) material->update_property(full_name, glm::vec2(val[0], val[1]));
                        else if (val.size() == 3) material->update_property(full_name, glm::vec3(val[0], val[1], val[2]));
                        else if (val.size() == 4)
                        {
                            const auto expected_type = static_cast<ShaderDataType>(binding_info->data_type);
                            if (expected_type == ShaderDataType::Mat2)
                                material->update_property(full_name, glm::mat2(val[0], val[1], val[2], val[3]));
                            else
                                material->update_property(full_name, glm::vec4(val[0], val[1], val[2], val[3]));
                        }
                        else if (val.size() == 9)
                            material->update_property(full_name, glm::mat3(
                                val[0], val[1], val[2],
                                val[3], val[4], val[5],
                                val[6], val[7], val[8]
                            ));
                        else if (val.size() == 16)
                            material->update_property(full_name, glm::mat4(
                                val[0], val[1], val[2], val[3],
                                val[4], val[5], val[6], val[7],
                                val[8], val[9], val[10], val[11],
                                val[12], val[13], val[14], val[15]
                            ));
                    }
                    else if constexpr (std::is_same_v<D, std::vector<std::int32_t>>)
                    {
                        if (val.size() == 2) material->update_property(full_name, glm::ivec2(val[0], val[1]));
                        else if (val.size() == 3) material->update_property(full_name, glm::ivec3(val[0], val[1], val[2]));
                        else if (val.size() == 4) material->update_property(full_name, glm::ivec4(val[0], val[1], val[2], val[3]));
                    }
                    else if constexpr (std::is_same_v<D, std::vector<std::uint32_t>>)
                    {
                        if (val.size() == 2) material->update_property(full_name, glm::uvec2(val[0], val[1]));
                        else if (val.size() == 3) material->update_property(full_name, glm::uvec3(val[0], val[1], val[2]));
                        else if (val.size() == 4) material->update_property(full_name, glm::uvec4(val[0], val[1], val[2], val[3]));
                    }
                    else if constexpr (std::is_same_v<D, std::vector<double>>)
                    {
                        if (val.size() == 2) material->update_property(full_name, glm::dvec2(val[0], val[1]));
                        else if (val.size() == 3) material->update_property(full_name, glm::dvec3(val[0], val[1], val[2]));
                        else if (val.size() == 4) material->update_property(full_name, glm::dvec4(val[0], val[1], val[2], val[3]));
                    }
                }, value);
            }
        }
    }

    Material* MaterialLoader::get_or_create_material(const std::string& name)
    {
        if (materials_.contains(name)) return materials_[name];

        if (definitions_.contains(name))
        {
            const auto& def      = definitions_[name];
            auto*       material = create_material_from_definition(def);
            if (material)
            {
                materials_[name] = material;
                bind_engine_resources(material);
                setup_material_from_definition(material, def);
                // Log::trace("Created on-demand material: {}", name);
                return material;
            }
        }

        Log::warn("Material '{}' not found", name);
        return nullptr;
    }

    Material* MaterialLoader::get_material(const std::string& name) const
    {
        const auto it = materials_.find(name);
        if (it != materials_.end()) { return it->second; }
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

        // Log::trace("Registered custom material: {}", name);
    }

    void MaterialLoader::update_time_ubo(const float time, const float delta_time) const
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
