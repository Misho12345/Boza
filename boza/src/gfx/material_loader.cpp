module boza.gfx.material_loader;

import boza.rhi;
import boza.rhi.render_context;

import boza.gfx;
import boza.gfx.texture_loader;
import boza.gfx.sampler_loader;

import boza.detail;

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

        camera_ubo_.emplace(
            sizeof(CameraUBO),
            BufferUsage::Uniform,
            ResourceAccessMode::Dynamic
        );

        light_ubo_.emplace(
            sizeof(LightUBO),
            BufferUsage::Uniform,
            ResourceAccessMode::Static
        );

        static constexpr LightUBO light_data{
            .light_position = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f),
            .light_color = glm::vec4(1.0f, 1.0f, 1.0f, 2.0f),
            .view_pos = glm::vec4(0.0f, 0.0f, 5.0f, 1.0f)
        };
        light_ubo_->upload(&light_data, sizeof(LightUBO), 0);

        time_ubo_.emplace(
            sizeof(TimeUBO),
            BufferUsage::Uniform,
            ResourceAccessMode::Dynamic
        );

        initialized_ = true;
        return true;
    }

    void MaterialLoader::shutdown()
    {
        if (!initialized_) return;

        initialized_ = false;

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
                MaterialSettings settings{
                    .vertex_shader = def.vertex_shader,
                    .fragment_shader = def.fragment_shader,
                    .depth_compare_op = def.settings.depth_compare_op,
                    .depth_test_enable = def.settings.depth_test_enable,
                    .depth_write_enable = def.settings.depth_write_enable,
                    .cull_mode = def.settings.cull_mode,
                    .front_face = def.settings.front_face
                };

                Material mat = create(name, settings);
                auto [it, inserted] = materials_.try_emplace(name, std::move(mat));

                if (inserted)
                {
                    valid_pointers_.insert(&it->second);
                    it->second.set_cpu_cull_enabled(def.cpu_cull_enabled);
                    bind_engine_resources(&it->second);
                    setup(&it->second, def);
                }
                else
                {
                    Log::error("Failed to create material: {}", name);
                    if (inserted) materials_.erase(it);
                }
            }
        }
        return true;
    }

    std::optional<MaterialDefinition> MaterialLoader::load_material_definition(const fs::path& path)
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

            if (s.contains("cpu_cull") && s["cpu_cull"].is_boolean())
                def.cpu_cull_enabled = s["cpu_cull"].get<bool>();
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
                        properties[prop_name] = prop_val.get<std::int64_t>() >= 0 ? prop_val.get<std::uint32_t>() : prop_val.get<std::int32_t>();
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

    void MaterialLoader::bind_engine_resources(const Material* material) const
    {
        if (!material || material->descriptor_set_count() == 0) return;

        const bool has_per_frame_sets = !material->descriptor_sets_per_frame_.empty();

        std::uint32_t frame_index = 0;
        if (has_per_frame_sets)
        {
            if (const auto* swapchain = rhi::RenderContext::swapchain())
                frame_index = swapchain->current_frame();

            frame_index %= static_cast<std::uint32_t>(material->descriptor_sets_per_frame_.size());
        }

        auto try_bind_ubo = [&](const std::string& ubo_name, const std::optional<Buffer>& buffer_opt, const std::size_t size)
        {
            if (!buffer_opt) return;

            const auto info = material->lookup_binding(ubo_name);
            if (!info.has_value() ||
                info->descriptor_type != static_cast<std::uint32_t>(rhi::DescriptorType::UniformBuffer))
                return;

            auto* rhi_buffer = static_cast<rhi::Buffer*>(buffer_opt->rhi_handle());
            if (!rhi_buffer) return;

            const rhi::DescriptorWrite write{
                .binding = info->binding,
                .array_element = 0,
                .type = rhi::DescriptorType::UniformBuffer,
                .info = rhi::UniformBuffer{
                    .buffer = rhi_buffer,
                    .offset = 0,
                    .range = static_cast<std::uint32_t>(size)
                }
            };

            if (has_per_frame_sets)
            {
                const auto& frame_sets = material->descriptor_sets_per_frame_[frame_index];
                if (info->set >= frame_sets.size()) return;

                auto* frame_set = static_cast<rhi::DescriptorSet*>(frame_sets[info->set]);
                if (!frame_set) return;

                std::array writes{ write };
                frame_set->update(writes);
                return;
            }

            auto* desc_set = static_cast<rhi::DescriptorSet*>(material->rhi_descriptor_set_handle(info->set));
            if (!desc_set) return;
            std::array writes{ write };
            desc_set->update(writes);
        };

        try_bind_ubo("cameraUBO", camera_ubo_, sizeof(CameraUBO));
        try_bind_ubo("lightUBO", light_ubo_, sizeof(LightUBO));
        try_bind_ubo("timeUBO", time_ubo_, sizeof(TimeUBO));
    }

    void MaterialLoader::setup(Material* material, const MaterialDefinition& def)
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

            Texture& texture = is_cubemap
                                   ? TextureLoader::instance().get_or_load_cubemap(tex_info.texture_file)
                                   : TextureLoader::instance().get_or_load(tex_info.texture_file);

            Sampler& sampler = SamplerLoader::instance().get_sampler(tex_info.sampler_file);

            material->update_texture(prop_name, texture, sampler);
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

                    if constexpr (std::same_as<D, bool>) material->update_property(full_name, static_cast<std::uint32_t>(val));
                    else if constexpr (std::same_as<D, std::int32_t> ||
                        std::same_as<D, std::uint32_t> ||
                        std::same_as<D, float> ||
                        std::same_as<D, double>) material->update_property(full_name, val);
                    else if constexpr (std::same_as<D, std::vector<float>>)
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
                    else if constexpr (std::same_as<D, std::vector<std::int32_t>>)
                    {
                        if (val.size() == 2) material->update_property(full_name, glm::ivec2(val[0], val[1]));
                        else if (val.size() == 3) material->update_property(full_name, glm::ivec3(val[0], val[1], val[2]));
                        else if (val.size() == 4) material->update_property(full_name, glm::ivec4(val[0], val[1], val[2], val[3]));
                    }
                    else if constexpr (std::same_as<D, std::vector<std::uint32_t>>)
                    {
                        if (val.size() == 2) material->update_property(full_name, glm::uvec2(val[0], val[1]));
                        else if (val.size() == 3) material->update_property(full_name, glm::uvec3(val[0], val[1], val[2]));
                        else if (val.size() == 4) material->update_property(full_name, glm::uvec4(val[0], val[1], val[2], val[3]));
                    }
                    else if constexpr (std::same_as<D, std::vector<double>>)
                    {
                        if (val.size() == 2) material->update_property(full_name, glm::dvec2(val[0], val[1]));
                        else if (val.size() == 3) material->update_property(full_name, glm::dvec3(val[0], val[1], val[2]));
                        else if (val.size() == 4) material->update_property(full_name, glm::dvec4(val[0], val[1], val[2], val[3]));
                    }
                }, value);
            }
        }
    }

    Material* MaterialLoader::try_get_material(const std::string_view name)
    {
        auto it = materials_.find(name);
        if (it != materials_.end()) return &it->second;

        std::string name_str{ name };

        auto def_it = definitions_.find(name_str);
        if (def_it != definitions_.end() && def_it->second.load_strategy == LoadStrategy::OnDemand)
        {
            const auto& def = def_it->second;
            // Log::trace("Loading on-demand material: {}", name);

            const MaterialSettings settings{
                .vertex_shader = def.vertex_shader,
                .fragment_shader = def.fragment_shader,
                .depth_compare_op = def.settings.depth_compare_op,
                .depth_test_enable = def.settings.depth_test_enable,
                .depth_write_enable = def.settings.depth_write_enable,
                .cull_mode = def.settings.cull_mode,
                .front_face = def.settings.front_face
            };

            Material mat = create(name, settings);
            auto [new_it, inserted] = materials_.try_emplace(name_str, std::move(mat));

            if (inserted && new_it->second.pipeline_)
            {
                valid_pointers_.insert(&new_it->second);
                new_it->second.set_cpu_cull_enabled(def.cpu_cull_enabled);
                bind_engine_resources(&new_it->second);
                setup(&new_it->second, def);
                return &new_it->second;
            }

            Log::error("Failed to create on-demand material: {}", name);
            if (inserted) materials_.erase(new_it);
            return nullptr;
        }

        return nullptr;
    }

    Material& MaterialLoader::get_or_create_material(const std::string_view name, const MaterialSettings& settings)
    {
        if (auto* existing = try_get_material(name)) return *existing;

        Material mat = create(name, settings);

        std::string name_str{ name };
        auto [it, inserted] = materials_.try_emplace(name_str, std::move(mat));

        if (!inserted || !it->second.pipeline_)
        {
            Log::error("Failed to create material: {}", name);
            if (inserted) materials_.erase(it);
            std::abort();
        }

        valid_pointers_.insert(&it->second);
        bind_engine_resources(&it->second);
        return it->second;
    }

    void MaterialLoader::destroy(const std::string_view name)
    {
        const auto it = materials_.find(name);
        if (it != materials_.end())
        {
            RenderingSystem::on_material_destroyed(&it->second);
            valid_pointers_.erase(&it->second);
            Log::trace("Destroyed material: {}", name);
            materials_.erase(it);
        }
        else Log::warn("Attempted to destroy non-existent material: {}", name);
    }

    bool MaterialLoader::exists(const Material* ptr) const
    {
        return ptr && valid_pointers_.contains(ptr);
    }

    Material MaterialLoader::create(
        const std::string_view name,
        const MaterialSettings& settings)
    {
        Material material{ name };

        if (settings.vertex_shader.empty() || settings.fragment_shader.empty())
        {
            Log::error("Material '{}': vertex or fragment shader name is empty", name);
            return material;
        }

        if (!rhi::RenderContext::initialized() || !rhi::RenderContext::device())
        {
            Log::error("Render context not initialized. Cannot create material.");
            return material;
        }

        auto* device          = rhi::RenderContext::device();
        auto  api             = rhi::RenderContext::api();
        auto* resource_cache  = rhi::RenderContext::resource_cache();
        auto* descriptor_pool = rhi::RenderContext::descriptor_pool();

        if (!resource_cache)
        {
            Log::error("ResourceCache not available in graphics context. Cannot create material.");
            return material;
        }

        if (!descriptor_pool)
        {
            Log::error("DescriptorPool not available in graphics context. Cannot create material.");
            return material;
        }

        const rhi::ShaderModuleDesc vert_desc{
            .device = device,
            .filename = settings.vertex_shader + ".vert",
            .stage = rhi::ShaderStage::Vertex
        };

        const rhi::ShaderModuleDesc frag_desc{
            .device = device,
            .filename = settings.fragment_shader + ".frag",
            .stage = rhi::ShaderStage::Fragment
        };

        const auto vert_shader_shared = resource_cache->get_or_create_shader(
            vert_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return create_shader_module(api, desc); });

        if (!vert_shader_shared)
        {
            Log::error("Failed to load vertex shader: {}", settings.vertex_shader);
            return material;
        }

        const auto frag_shader_shared = resource_cache->get_or_create_shader(
            frag_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return create_shader_module(api, desc); });

        if (!frag_shader_shared)
        {
            Log::error("Failed to load fragment shader: {}", settings.fragment_shader);
            return material;
        }

        rhi::ShaderModule* vert_shader = vert_shader_shared.get();
        rhi::ShaderModule* frag_shader = frag_shader_shared.get();

        rhi::GraphicsPipeline* pipeline        = nullptr;
        rhi::PipelineLayout*   pipeline_layout = nullptr;

        std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;

        const std::size_t settings_hash = settings.hash();
        const auto* cached = resource_cache->get_cached_pipeline(settings.vertex_shader, settings.fragment_shader, settings_hash);

        if (cached)
        {
            pipeline               = cached->pipeline;
            pipeline_layout        = cached->layout;
            descriptor_set_layouts = cached->descriptor_set_layouts;
        }
        else
        {
            rhi::PipelineBuilder builder(api, device, { vert_shader, frag_shader });

            if (!builder.build_descriptor_set_layouts())
            {
                Log::error("Failed to build descriptor set layouts for material");
                return material;
            }

            pipeline_layout = builder.build_pipeline_layout();
            if (!pipeline_layout)
            {
                Log::error("Failed to create pipeline layout for material");
                const auto& layouts = builder.get_descriptor_set_layouts();
                for (auto* layout : layouts) { if (layout) layout->destroy(); }
                return material;
            }

            const auto* swapchain = rhi::RenderContext::swapchain();
            if (!swapchain)
            {
                Log::error("Swapchain not available in graphics context. Cannot create material.");
                if (pipeline_layout) pipeline_layout->destroy();
                const auto& layouts = builder.get_descriptor_set_layouts();
                for (auto* layout : layouts) { if (layout) layout->destroy(); }
                return material;
            }

            const rhi::RasterizationState raster_state{
                .cull_mode = settings.cull_mode,
                .front_face = settings.front_face
            };

            const rhi::DepthStencilState depth_state{
                .depth_test_enable = settings.depth_test_enable,
                .depth_write_enable = settings.depth_write_enable,
                .depth_compare_op = settings.depth_compare_op
            };

            pipeline = builder.build_graphics_pipeline(swapchain, swapchain->depth_format(), raster_state, depth_state);

            if (!pipeline)
            {
                Log::error("Failed to create graphics pipeline for material");
                if (pipeline_layout) pipeline_layout->destroy();
                const auto& layouts = builder.get_descriptor_set_layouts();
                for (auto* layout : layouts) { if (layout) layout->destroy(); }
                return material;
            }

            descriptor_set_layouts = builder.get_descriptor_set_layouts();

            resource_cache->cache_pipeline(
                settings.vertex_shader, settings.fragment_shader, settings_hash, {
                    .pipeline = pipeline,
                    .layout = pipeline_layout,
                    .descriptor_set_layouts = descriptor_set_layouts
                });
        }

        const std::uint32_t frame_count = std::max<std::uint32_t>(rhi::RenderContext::frames_in_flight(), 1u);

        std::vector<std::vector<rhi::DescriptorSet*>> descriptor_sets_per_frame(frame_count);
        for (std::uint32_t frame_index = 0; frame_index < frame_count; ++frame_index)
        {
            auto& descriptor_sets = descriptor_sets_per_frame[frame_index];
            descriptor_sets.reserve(descriptor_set_layouts.size());

            for (auto* layout : descriptor_set_layouts)
            {
                auto* desc_set = descriptor_pool->allocate_descriptor_set(layout);
                if (!desc_set)
                {
                    Log::error("Failed to allocate descriptor set for material (frame {})", frame_index);
                    return material;
                }

                descriptor_sets.push_back(desc_set);
            }
        }

        material.pipeline_ = pipeline;
        material.pipeline_layout_ = pipeline_layout;
        material.descriptor_set_layouts_.reserve(descriptor_set_layouts.size());
        for (auto* layout : descriptor_set_layouts)
        {
            material.descriptor_set_layouts_.push_back(layout);
        }
        material.descriptor_sets_per_frame_.reserve(frame_count);
        for (const auto& frame_sets : descriptor_sets_per_frame)
        {
            auto& dst_frame_sets = material.descriptor_sets_per_frame_.emplace_back();
            dst_frame_sets.reserve(frame_sets.size());

            for (auto* set : frame_sets)
            {
                dst_frame_sets.push_back(set);
            }
        }

        if (!material.descriptor_sets_per_frame_.empty())
        {
            material.descriptor_sets_.reserve(material.descriptor_sets_per_frame_.front().size());

            for (auto* set : material.descriptor_sets_per_frame_.front())
            {
                material.descriptor_sets_.push_back(set);
            }
        }

        material.descriptor_pool_ = descriptor_pool;

        auto* reflection = new rhi::DescriptorReflection();
        std::array shaders{ vert_shader, frag_shader };
        reflection->build_from_shaders(shaders);
        material.reflection_ = reflection;

        return material;
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
