module boza.gfx;

import boza.rhi;
import :material_loader;

import :texture_loader;
import :sampler_loader;

import boza.detail;

namespace boza::gfx
{
    namespace
    {
        constexpr std::uint32_t max_point_lights_ = 1024;
        constexpr std::uint32_t max_directional_lights_ = 8;
        constexpr std::uint32_t max_spot_lights_ = 128;
        constexpr std::uint32_t max_shadow_casters_ = 65'536;
        constexpr std::uint32_t min_cluster_light_indices_capacity_ = 65'536;
        constexpr std::uint32_t max_point_shadow_maps_ = 4;
        constexpr std::uint32_t max_spot_shadow_maps_ = 16;
        constexpr std::uint32_t directional_shadow_cascade_count_ = 4;
        constexpr float directional_shadow_extent_ = 260.0f;
        constexpr float point_shadow_near_plane_ = 0.05f;
        constexpr float spot_shadow_near_plane_ = 0.05f;
        constexpr std::uint32_t cluster_tile_size_ = 64u;
        constexpr std::uint32_t cluster_z_slice_count_ = 24u;
        constexpr std::uint32_t cluster_index_budget_per_cluster_ = 64u;
        constexpr std::string_view frame_depth_texture_name_{ "__boza_frame_depth" };
        constexpr std::string_view ssao_texture_name_{ "__boza_ssao" };
        constexpr std::string_view renderer_sampler_name_{ "__boza_renderer_sampler" };
        constexpr std::string_view directional_shadow_map_name_{ "__boza_directional_shadow_map" };
        constexpr std::string_view directional_shadow_sampler_name_{ "__boza_directional_shadow_sampler" };
        constexpr std::string_view point_shadow_map_name_{ "__boza_point_shadow_map" };
        constexpr std::string_view point_shadow_sampler_name_{ "__boza_point_shadow_sampler" };
        constexpr std::string_view spot_shadow_map_name_{ "__boza_spot_shadow_map" };
        constexpr std::string_view spot_shadow_sampler_name_{ "__boza_spot_shadow_sampler" };

        [[nodiscard]] glm::uvec3 compute_cluster_grid_size(
            const std::uint32_t screen_width,
            const std::uint32_t screen_height)
        {
            return {
                std::max((screen_width + cluster_tile_size_ - 1u) / cluster_tile_size_, 1u),
                std::max((screen_height + cluster_tile_size_ - 1u) / cluster_tile_size_, 1u),
                cluster_z_slice_count_
            };
        }

        [[nodiscard]] std::uint32_t cluster_count(const glm::uvec3& cluster_grid_size)
        {
            return cluster_grid_size.x * cluster_grid_size.y * cluster_grid_size.z;
        }

        [[nodiscard]] float safe_range(const float value, const float fallback)
        {
            return value > 0.001f ? value : fallback;
        }

        [[nodiscard]] glm::vec3 safe_direction(glm::vec3 direction)
        {
            if (glm::length2(direction) <= 1e-8f) return glm::vec3{ 0.0f, -1.0f, 0.0f };
            return glm::normalize(direction);
        }

        [[nodiscard]] std::array<glm::vec3, 8> build_camera_slice_corners(
            const glm::mat4& view,
            const glm::mat4& projection,
            const float near_clip,
            const float far_clip)
        {
            const glm::mat4 inv_view = glm::inverse(view);
            const glm::vec3 camera_position = glm::vec3{ inv_view[3] };
            const glm::vec3 camera_right = safe_direction(glm::vec3{ inv_view[0] });
            const glm::vec3 camera_up = safe_direction(glm::vec3{ inv_view[1] });
            const glm::vec3 camera_forward = safe_direction(glm::vec3{ inv_view[2] });

            const float tan_half_fov_y = 1.0f / std::max(projection[1][1], 1e-5f);
            const float tan_half_fov_x = 1.0f / std::max(projection[0][0], 1e-5f);

            const glm::vec3 near_center = camera_position + camera_forward * near_clip;
            const glm::vec3 far_center = camera_position + camera_forward * far_clip;

            const float near_half_height = near_clip * tan_half_fov_y;
            const float near_half_width = near_clip * tan_half_fov_x;
            const float far_half_height = far_clip * tan_half_fov_y;
            const float far_half_width = far_clip * tan_half_fov_x;

            return {
                near_center - camera_right * near_half_width - camera_up * near_half_height,
                near_center + camera_right * near_half_width - camera_up * near_half_height,
                near_center + camera_right * near_half_width + camera_up * near_half_height,
                near_center - camera_right * near_half_width + camera_up * near_half_height,
                far_center - camera_right * far_half_width - camera_up * far_half_height,
                far_center + camera_right * far_half_width - camera_up * far_half_height,
                far_center + camera_right * far_half_width + camera_up * far_half_height,
                far_center - camera_right * far_half_width + camera_up * far_half_height
            };
        }

        [[nodiscard]] glm::mat4 build_directional_shadow_view_projection(
            const glm::mat4& view,
            const glm::mat4& projection,
            const float near_clip,
            const float far_clip,
            glm::vec3 light_direction,
            const std::uint32_t shadow_map_size,
            const float light_distance)
        {
            light_direction = safe_direction(light_direction);
            const auto corners = build_camera_slice_corners(view, projection, near_clip, far_clip);
            const glm::vec3 caster_extrusion = -light_direction * light_distance;

            glm::vec3 up{ 0.0f, 1.0f, 0.0f };
            if (std::abs(glm::dot(up, light_direction)) > 0.92f) up = glm::vec3{ 0.0f, 0.0f, 1.0f };

            glm::vec3 slice_center{ 0.0f };
            for (const glm::vec3& corner : corners)
                slice_center += corner;
            slice_center /= static_cast<float>(corners.size());

            const glm::mat4 light_view = glm::lookAt(
                slice_center - light_direction * light_distance,
                slice_center,
                up);

            glm::vec3 light_min{ std::numeric_limits<float>::max() };
            glm::vec3 light_max{ std::numeric_limits<float>::lowest() };
            for (const glm::vec3& corner : corners)
            {
                const glm::vec3 light_space = glm::vec3{ light_view * glm::vec4{ corner, 1.0f } };
                light_min = glm::min(light_min, light_space);
                light_max = glm::max(light_max, light_space);

                // Include blocker space outside the visible receiver slice so shadows stay present
                // when the caster slips just off-camera but still projects into the view.
                const glm::vec3 extruded_light_space = glm::vec3{ light_view * glm::vec4{ corner + caster_extrusion, 1.0f } };
                light_min = glm::min(light_min, extruded_light_space);
                light_max = glm::max(light_max, extruded_light_space);
            }

            const glm::vec3 slice_center_light_space = glm::vec3{ light_view * glm::vec4{ slice_center, 1.0f } };
            const glm::vec2 half_extent = glm::max(
                glm::vec2{ (light_max.x - light_min.x) * 0.5f, (light_max.y - light_min.y) * 0.5f },
                glm::vec2{ 8.0f });
            const float slice_radius = std::max(half_extent.x, half_extent.y);
            const float xy_padding = std::max(18.0f, slice_radius * 0.22f);
            const float square_half_extent = slice_radius + xy_padding;
            const float texel_size = (square_half_extent * 2.0f) / static_cast<float>(std::max(shadow_map_size, 1u));
            const glm::vec2 snapped_center =
                glm::round(glm::vec2{ slice_center_light_space } / std::max(texel_size, 1e-5f)) * texel_size;
            const glm::vec2 snapped_min = snapped_center - glm::vec2{ square_half_extent };
            const glm::vec2 snapped_max = snapped_center + glm::vec2{ square_half_extent };

            const float depth_padding = std::max(128.0f, light_distance * 0.55f);
            const float near_plane = light_min.z - depth_padding;
            const float far_plane = std::max(light_max.z + depth_padding, near_plane + 1.0f);

            const glm::mat4 light_projection = glm::ortho(
                snapped_min.x,
                snapped_max.x,
                snapped_min.y,
                snapped_max.y,
                near_plane,
                far_plane);

            return light_projection * light_view;
        }

        [[nodiscard]] std::array<float, directional_shadow_cascade_count_> build_directional_shadow_splits(
            const float near_clip,
            const float far_clip)
        {
            std::array<float, directional_shadow_cascade_count_> splits{};
            constexpr float lambda = 0.72f;

            for (std::uint32_t i = 0; i < directional_shadow_cascade_count_; ++i)
            {
                const float p = static_cast<float>(i + 1u) / static_cast<float>(directional_shadow_cascade_count_);
                const float logarithmic = near_clip * std::pow(far_clip / near_clip, p);
                const float uniform = near_clip + (far_clip - near_clip) * p;
                splits[i] = std::lerp(uniform, logarithmic, lambda);
            }

            return splits;
        }

        [[nodiscard]] glm::vec3 camera_forward_from_view(const glm::mat4& view)
        {
            const glm::mat4 inv_view = glm::inverse(view);
            return safe_direction(glm::vec3{ inv_view[2] });
        }

        [[nodiscard]] std::array<glm::mat4, 6> build_point_shadow_view_projections(
            const glm::vec3& position,
            const float range)
        {
            const glm::mat4 projection = glm::perspective(
                glm::radians(90.0f),
                1.0f,
                point_shadow_near_plane_,
                std::max(range, point_shadow_near_plane_ + 1.0f));

            return {
                projection * glm::lookAt(position, position + glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, -1.0f, 0.0f }),
                projection * glm::lookAt(position, position + glm::vec3{ -1.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, -1.0f, 0.0f }),
                projection * glm::lookAt(position, position + glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 0.0f, 0.0f, 1.0f }),
                projection * glm::lookAt(position, position + glm::vec3{ 0.0f, -1.0f, 0.0f }, glm::vec3{ 0.0f, 0.0f, -1.0f }),
                projection * glm::lookAt(position, position + glm::vec3{ 0.0f, 0.0f, 1.0f }, glm::vec3{ 0.0f, -1.0f, 0.0f }),
                projection * glm::lookAt(position, position + glm::vec3{ 0.0f, 0.0f, -1.0f }, glm::vec3{ 0.0f, -1.0f, 0.0f })
            };
        }

        [[nodiscard]] glm::mat4 build_spot_shadow_view_projection(
            const glm::vec3& position,
            glm::vec3 direction,
            const float outer_angle,
            const float range)
        {
            direction = safe_direction(direction);

            glm::vec3 up{ 0.0f, 1.0f, 0.0f };
            if (std::abs(glm::dot(up, direction)) > 0.92f) up = glm::vec3{ 0.0f, 0.0f, 1.0f };

            const glm::mat4 projection = glm::perspective(
                std::max(outer_angle * 2.0f, glm::radians(5.0f)),
                1.0f,
                spot_shadow_near_plane_,
                std::max(range, spot_shadow_near_plane_ + 1.0f));

            return projection * glm::lookAt(position, position + direction, up);
        }
    }

    static MaterialSettings settings_from_definition(const MaterialDefinition& def)
    {
        return MaterialSettings{
            .vertex_shader = def.vertex_shader,
            .fragment_shader = def.fragment_shader,
            .depth_compare_op = def.settings.depth_compare_op,
            .depth_test_enable = def.settings.depth_test_enable,
            .depth_write_enable = def.settings.depth_write_enable,
            .cull_mode = def.settings.cull_mode,
            .front_face = def.settings.front_face
        };
    }

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

        lighting_ubo_.emplace(
            sizeof(LightingUBO),
            BufferUsage::Uniform,
            ResourceAccessMode::Dynamic
        );

        point_lights_buffer_.emplace(
            max_point_lights_ * sizeof(GpuPointLight),
            BufferUsage::Storage,
            ResourceAccessMode::Dynamic
        );

        directional_lights_buffer_.emplace(
            max_directional_lights_ * sizeof(GpuDirectionalLight),
            BufferUsage::Storage,
            ResourceAccessMode::Dynamic
        );

        spot_lights_buffer_.emplace(
            max_spot_lights_ * sizeof(GpuSpotLight),
            BufferUsage::Storage,
            ResourceAccessMode::Dynamic
        );

        cluster_grid_size_ = compute_cluster_grid_size(
            swapchain_ ? swapchain_->width() : 1u,
            swapchain_ ? swapchain_->height() : 1u);
        ensure_cluster_buffers(cluster_grid_size_);

        cluster_light_counter_buffer_.emplace(
            sizeof(GpuClusterLightCounter),
            BufferUsage::Storage,
            ResourceAccessMode::Dynamic
        );

        shadow_casters_buffer_.emplace(
            max_shadow_casters_ * sizeof(GpuShadowCaster),
            BufferUsage::Storage,
            ResourceAccessMode::Dynamic
        );

        directional_shadow_map_size_ = 2048u;
        directional_shadow_map_ = &Texture::create(
            directional_shadow_map_name_,
            TextureSettings{
                .type = TextureType::Texture2DArray,
                .format = TextureFormat::DEPTH32F,
                .access_mode = ResourceAccessMode::Static,
                .width = directional_shadow_map_size_,
                .height = directional_shadow_map_size_,
                .depth = directional_shadow_cascade_count_,
                .usage_flags = TextureUsage::Sampled | TextureUsage::DepthStencilAttachment
            });
        if (directional_shadow_map_)
            directional_shadow_map_->transition_layout(TextureLayout::Undefined, TextureLayout::ShaderReadOnly);

        directional_shadow_sampler_ = &Sampler::create(
            directional_shadow_sampler_name_,
            SamplerFilter::Linear,
            SamplerWrap::ClampToEdge,
            SamplerWrap::ClampToEdge,
            SamplerWrap::ClampToEdge,
            SamplerFilter::Nearest,
            0.0f,
            0.0f,
            0.0f,
            1.0f);

        point_shadow_map_size_ = 1024u;
        point_shadow_map_ = &Texture::create(
            point_shadow_map_name_,
            TextureSettings{
                .type = TextureType::Texture2DArray,
                .format = TextureFormat::DEPTH32F,
                .access_mode = ResourceAccessMode::Static,
                .width = point_shadow_map_size_,
                .height = point_shadow_map_size_,
                .depth = max_point_shadow_maps_ * 6u,
                .usage_flags = TextureUsage::Sampled | TextureUsage::DepthStencilAttachment
            });
        if (point_shadow_map_)
            point_shadow_map_->transition_layout(TextureLayout::Undefined, TextureLayout::ShaderReadOnly);

        point_shadow_sampler_ = &Sampler::create(
            point_shadow_sampler_name_,
            SamplerFilter::Linear,
            SamplerWrap::ClampToEdge,
            SamplerWrap::ClampToEdge,
            SamplerWrap::ClampToEdge,
            SamplerFilter::Nearest,
            0.0f,
            0.0f,
            0.0f,
            1.0f);

        point_shadow_matrices_buffer_.emplace(
            max_point_shadow_maps_ * 6u * sizeof(glm::mat4),
            BufferUsage::Storage,
            ResourceAccessMode::Dynamic);

        spot_shadow_matrices_buffer_.emplace(
            max_spot_shadow_maps_ * sizeof(glm::mat4),
            BufferUsage::Storage,
            ResourceAccessMode::Dynamic);

        spot_shadow_map_size_ = 1024u;
        spot_shadow_map_ = &Texture::create(
            spot_shadow_map_name_,
            TextureSettings{
                .type = TextureType::Texture2DArray,
                .format = TextureFormat::DEPTH32F,
                .access_mode = ResourceAccessMode::Static,
                .width = spot_shadow_map_size_,
                .height = spot_shadow_map_size_,
                .depth = max_spot_shadow_maps_,
                .usage_flags = TextureUsage::Sampled | TextureUsage::DepthStencilAttachment
            });
        if (spot_shadow_map_)
            spot_shadow_map_->transition_layout(TextureLayout::Undefined, TextureLayout::ShaderReadOnly);

        spot_shadow_sampler_ = &Sampler::create(
            spot_shadow_sampler_name_,
            SamplerFilter::Linear,
            SamplerWrap::ClampToEdge,
            SamplerWrap::ClampToEdge,
            SamplerWrap::ClampToEdge,
            SamplerFilter::Nearest,
            0.0f,
            0.0f,
            0.0f,
            1.0f);

        renderer_sampler_ = &Sampler::create(
            renderer_sampler_name_,
            SamplerFilter::Linear,
            SamplerWrap::ClampToEdge,
            SamplerWrap::ClampToEdge,
            SamplerWrap::ClampToEdge,
            SamplerFilter::Nearest,
            0.0f,
            0.0f,
            0.0f,
            1.0f);

        begin_light_update(
            glm::vec3{ 0.0f, 0.0f, 5.0f },
            glm::identity<glm::mat4>(),
            glm::identity<glm::mat4>(),
            1,
            1,
            0.1f,
            1000.0f);
        upload_light_buffers();

        time_ubo_.emplace(
            sizeof(TimeUBO),
            BufferUsage::Uniform,
            ResourceAccessMode::Dynamic
        );

        if (swapchain_)
            ensure_frame_render_targets(swapchain_->width(), swapchain_->height());

        initialized_ = true;
        return true;
    }

    void MaterialLoader::ensure_cluster_buffers(const glm::uvec3& cluster_grid_size)
    {
        const std::size_t required_cluster_records = cluster_count(cluster_grid_size);
        if (required_cluster_records > cluster_record_capacity_)
        {
            cluster_records_buffer_.emplace(
                required_cluster_records * sizeof(GpuClusterRecord),
                BufferUsage::Storage,
                ResourceAccessMode::Dynamic);
            cluster_record_capacity_ = required_cluster_records;
        }

        const std::size_t required_cluster_light_indices = std::max(
            required_cluster_records * static_cast<std::size_t>(cluster_index_budget_per_cluster_),
            static_cast<std::size_t>(min_cluster_light_indices_capacity_));

        if (required_cluster_light_indices > cluster_light_index_capacity_)
        {
            cluster_light_indices_buffer_.emplace(
                required_cluster_light_indices * sizeof(std::uint32_t),
                BufferUsage::Storage,
                ResourceAccessMode::Dynamic);
            cluster_light_index_capacity_ = required_cluster_light_indices;
        }
    }

    void MaterialLoader::shutdown()
    {
        if (!initialized_) return;

        initialized_ = false;

        materials_.clear();

        if (directional_shadow_map_) Texture::destroy(directional_shadow_map_name_);
        if (directional_shadow_sampler_) Sampler::destroy(directional_shadow_sampler_name_);
        if (point_shadow_map_) Texture::destroy(point_shadow_map_name_);
        if (point_shadow_sampler_) Sampler::destroy(point_shadow_sampler_name_);
        if (spot_shadow_map_) Texture::destroy(spot_shadow_map_name_);
        if (spot_shadow_sampler_) Sampler::destroy(spot_shadow_sampler_name_);
        if (frame_depth_texture_) Texture::destroy(frame_depth_texture_name_);
        if (ssao_texture_) Texture::destroy(ssao_texture_name_);
        if (renderer_sampler_) Sampler::destroy(renderer_sampler_name_);
        directional_shadow_map_ = nullptr;
        directional_shadow_sampler_ = nullptr;
        directional_shadow_enabled_ = false;
        point_shadow_map_ = nullptr;
        point_shadow_sampler_ = nullptr;
        point_shadow_enabled_ = false;
        spot_shadow_map_ = nullptr;
        spot_shadow_sampler_ = nullptr;
        spot_shadow_enabled_ = false;
        frame_depth_texture_ = nullptr;
        ssao_texture_ = nullptr;
        renderer_sampler_ = nullptr;
        frame_render_target_width_ = 0;
        frame_render_target_height_ = 0;
        ++frame_render_target_revision_;

        point_shadow_matrices_buffer_.reset();
        spot_shadow_matrices_buffer_.reset();
        time_ubo_.reset();
        shadow_casters_buffer_.reset();
        cluster_light_indices_buffer_.reset();
        cluster_light_counter_buffer_.reset();
        cluster_records_buffer_.reset();
        spot_lights_buffer_.reset();
        directional_lights_buffer_.reset();
        point_lights_buffer_.reset();
        lighting_ubo_.reset();
        camera_ubo_.reset();

        shadow_casters_.clear();
        point_shadow_sources_.clear();
        spot_shadow_sources_.clear();
        point_shadow_view_projections_.clear();
        spot_shadow_view_projections_.clear();
        cluster_light_indices_.clear();
        cluster_records_.clear();
        spot_lights_.clear();
        directional_lights_.clear();
        point_lights_.clear();

        device_          = nullptr;
        swapchain_       = nullptr;
        descriptor_pool_ = nullptr;
        resource_cache_ = nullptr;
        api_             = {};

        definitions_.clear();
    }

    bool MaterialLoader::ensure_frame_render_targets(
        const std::uint32_t width,
        const std::uint32_t height)
    {
        const std::uint32_t resolved_width = std::max(width, 1u);
        const std::uint32_t resolved_height = std::max(height, 1u);
        const bool size_changed =
            frame_render_target_width_ != resolved_width ||
            frame_render_target_height_ != resolved_height;

        if (!size_changed && frame_depth_texture_)
        {
            return false;
        }

        if (frame_depth_texture_) Texture::destroy(frame_depth_texture_name_);
        if (ssao_texture_) Texture::destroy(ssao_texture_name_);

        frame_depth_texture_ = &Texture::create(
            frame_depth_texture_name_,
            TextureSettings{
                .type = TextureType::Texture2D,
                .format = TextureFormat::DEPTH32F,
                .access_mode = ResourceAccessMode::Dynamic,
                .width = resolved_width,
                .height = resolved_height,
                .depth = 1u,
                .usage_flags = TextureUsage::Sampled | TextureUsage::DepthStencilAttachment
            });

        ssao_texture_ = &Texture::create(
            ssao_texture_name_,
            TextureSettings{
                .type = TextureType::Texture2D,
                .format = TextureFormat::RGBA8,
                .access_mode = ResourceAccessMode::Dynamic,
                .width = resolved_width,
                .height = resolved_height,
                .depth = 1u,
                .usage_flags = TextureUsage::Sampled | TextureUsage::Storage
            });

        frame_render_target_width_ = resolved_width;
        frame_render_target_height_ = resolved_height;
        ++frame_render_target_revision_;

        return true;
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
                Material mat = create(name, settings_from_definition(def));
                auto [material, inserted] = materials_.try_emplace(name, std::move(mat));

                if (inserted && material && material->pipeline_)
                {
                    bind_engine_resources(material);
                    setup(material, def);
                }
                else
                {
                    Log::error("Failed to create material: {}", name);
                    if (inserted) materials_.erase(name);
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

        def.name = detail::AssetPaths::material_id_from_path(path);
        if (def.name.empty())
        {
            Log::error("Failed to derive material id from path: {}", path.string());
            return std::nullopt;
        }

        if (!j.contains("vertex_shader") || !j["vertex_shader"].is_string())
        {
            Log::error("Material {} missing 'vertex_shader' string", def.name);
            return std::nullopt;
        }
        def.vertex_shader = detail::AssetPaths::normalize_resource_id(j["vertex_shader"].get<std::string>());
        if (def.vertex_shader.empty())
        {
            Log::error("Material {} has invalid 'vertex_shader' path", def.name);
            return std::nullopt;
        }

        if (!j.contains("fragment_shader") || !j["fragment_shader"].is_string())
        {
            Log::error("Material {} missing 'fragment_shader' string", def.name);
            return std::nullopt;
        }
        def.fragment_shader = detail::AssetPaths::normalize_resource_id(j["fragment_shader"].get<std::string>());
        if (def.fragment_shader.empty())
        {
            Log::error("Material {} has invalid 'fragment_shader' path", def.name);
            return std::nullopt;
        }

        if (j.contains("load_strategy") && j["load_strategy"].is_string())
        {
            const std::string strategy = j["load_strategy"].get<std::string>();
            if (strategy == "game_load") def.load_strategy = LoadStrategy::GameLoad;
            else if (strategy == "on_demand") def.load_strategy = LoadStrategy::OnDemand;
            else
            {
                Log::warn("Material {} has unknown load_strategy '{}', defaulting to game_load", def.name, strategy);
            }
        }

        if (j.contains("shadow_only") && j["shadow_only"].is_boolean())
            def.shadow_only = j["shadow_only"].get<bool>();

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
                    if (val.contains("texture") && val["texture"].is_string())
                    {
                        info.texture_file = detail::AssetPaths::normalize_resource_id(val["texture"].get<std::string>());
                        if (info.texture_file.empty())
                            Log::warn("Material {} has invalid texture path for '{}'", def.name, key);
                    }
                    if (val.contains("sampler") && val["sampler"].is_string())
                    {
                        info.sampler_file = detail::AssetPaths::normalize_resource_id(val["sampler"].get<std::string>());
                        if (info.sampler_file.empty())
                            Log::warn("Material {} has invalid sampler path for '{}'", def.name, key);
                    }
                }

                def.textures[key] = info;
            }
        }

        if (j.contains("params") && j["params"].is_object())
        {
            for (const auto& [ubo_name, ubo_obj] : j["params"].items())
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
                        properties[prop_name] =
                            prop_val.get<std::int64_t>() >= 0
                                ? prop_val.get<std::uint32_t>()
                                : prop_val.get<std::int32_t>();
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

    void MaterialLoader::bind_engine_resources(Material* material) const
    {
        if (!material || material->descriptor_set_count() == 0) return;

        auto try_bind_ubo = [&](const std::string& ubo_name, const std::optional<Buffer>& buffer_opt)
        {
            if (!buffer_opt) return;

            const auto info = material->lookup_binding(ubo_name);
            if (!info.has_value() ||
                info->descriptor_type != static_cast<std::uint32_t>(rhi::DescriptorType::UniformBuffer))
            {
                return;
            }

            material->update_buffer(ubo_name, *buffer_opt);
        };

        auto try_bind_buffer = [&](const std::string& binding_name, const std::optional<Buffer>& buffer_opt)
        {
            if (!buffer_opt) return;

            if (!material->lookup_binding(binding_name).has_value()) return;
            material->update_buffer(binding_name, *buffer_opt);
        };

        try_bind_ubo("cameraUBO", camera_ubo_);
        try_bind_ubo("lightingUBO", lighting_ubo_);
        try_bind_ubo("timeUBO", time_ubo_);

        try_bind_buffer("pointLights", point_lights_buffer_);
        try_bind_buffer("directionalLights", directional_lights_buffer_);
        try_bind_buffer("spotLights", spot_lights_buffer_);
        try_bind_buffer("clusterRecords", cluster_records_buffer_);
        try_bind_buffer("clusterLightIndices", cluster_light_indices_buffer_);
        try_bind_buffer("pointShadowMatrices", point_shadow_matrices_buffer_);
        try_bind_buffer("spotShadowMatrices", spot_shadow_matrices_buffer_);
    }

    void MaterialLoader::log_cluster_cull_feedback()
    {
        if (!cluster_light_counter_buffer_) return;

        GpuClusterLightCounter feedback{};
        cluster_light_counter_buffer_->read_back(&feedback, sizeof(feedback), 0);
        if (feedback.overflow_count == 0) return;

        if (Time::frame_count() % 60u != 0u) return;

        Log::warn(
            "Clustered light list overflow detected in the previous frame: overflowing_clusters={} requested_indices={} capacity={}",
            feedback.overflow_count,
            feedback.next_index,
            cluster_light_index_capacity_);
    }

    void MaterialLoader::setup(Material* material, const MaterialDefinition& def)
    {
        if (!TextureLoader::instance().initialized()) return;
        if (!SamplerLoader::instance().initialized()) return;

        material->set_shadow_only(def.shadow_only);

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

                    if constexpr (std::same_as<D, bool>)
                    {
                        material->update_property(full_name, static_cast<std::uint32_t>(val));
                    }
                    else if constexpr (
                        std::same_as<D, std::int32_t> ||
                        std::same_as<D, std::uint32_t> ||
                        std::same_as<D, float> ||
                        std::same_as<D, double>)
                    {
                        material->update_property(full_name, val);
                    }
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
        if (auto* material = materials_.find_ptr(name)) return material;

        std::string name_str{ name };

        auto def_it = definitions_.find(name_str);
        if (def_it != definitions_.end() && def_it->second.load_strategy == LoadStrategy::OnDemand)
        {
            const auto& def = def_it->second;
            // Log::trace("Loading on-demand material: {}", name);

            Material mat = create(name, settings_from_definition(def));
            auto [material, inserted] = materials_.try_emplace(name_str, std::move(mat));

            if (inserted && material && material->pipeline_)
            {
                bind_engine_resources(material);
                setup(material, def);
                return material;
            }

            Log::error("Failed to create on-demand material: {}", name);
            if (inserted) materials_.erase(name_str);
            return nullptr;
        }

        return nullptr;
    }

    Material& MaterialLoader::get_or_create_material(const std::string_view name, const MaterialSettings& settings)
    {
        if (auto* existing = try_get_material(name)) return *existing;

        Material mat = create(name, settings);

        const std::string name_str{ name };
        auto [material, inserted] = materials_.try_emplace(name_str, std::move(mat));

        if (!inserted) return *material;

        if (!material->pipeline_)
        {
            Log::error("Failed to create material: {}", name);

            if (auto* default_material = try_get_material("default"))
            {
                Log::warn("Falling back to default material after '{}' creation failure", name);
                materials_.erase(name_str);
                return *default_material;
            }

            Log::error("No default material available; keeping '{}' in invalid state", name);
            return *material;
        }

        bind_engine_resources(material);
        return *material;
    }

    void MaterialLoader::destroy(const std::string_view name)
    {
        if (auto material = materials_.take(name))
        {
            RenderingSystem::on_material_destroyed(material.get());
            Log::trace("Destroyed material: {}", name);
        }
        else Log::warn("Attempted to destroy non-existent material: {}", name);
    }

    bool MaterialLoader::exists(const Material* ptr) const
    {
        return materials_.exists(ptr);
    }

    Material MaterialLoader::create(
        const std::string_view name,
        const MaterialSettings& settings)
    {
        auto& loader = instance();
        Material material{ name };
        material.settings_ = settings;

        const std::string vertex_shader_id = detail::AssetPaths::normalize_resource_id(settings.vertex_shader);
        const std::string fragment_shader_id = detail::AssetPaths::normalize_resource_id(settings.fragment_shader);

        if (vertex_shader_id.empty() || fragment_shader_id.empty())
        {
            Log::error("Material '{}': vertex or fragment shader name is empty", name);
            return material;
        }

        if (!loader.initialized_ || !loader.device_)
        {
            Log::error("MaterialLoader is not initialized. Cannot create material.");
            return material;
        }

        auto* device          = loader.device_;
        auto  api             = loader.api_;
        auto* resource_cache  = loader.resource_cache_;
        auto* descriptor_pool = loader.descriptor_pool_;

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
            .filename = vertex_shader_id + ".vert",
            .stage = rhi::ShaderStage::Vertex
        };

        const rhi::ShaderModuleDesc frag_desc{
            .device = device,
            .filename = fragment_shader_id + ".frag",
            .stage = rhi::ShaderStage::Fragment
        };

        const auto vert_shader_shared = resource_cache->get_or_create_shader(
            vert_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return create_shader_module(api, desc); });

        if (!vert_shader_shared)
        {
            Log::error("Failed to load vertex shader: {}", vertex_shader_id);
            return material;
        }

        const auto frag_shader_shared = resource_cache->get_or_create_shader(
            frag_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return create_shader_module(api, desc); });

        if (!frag_shader_shared)
        {
            Log::error("Failed to load fragment shader: {}", fragment_shader_id);
            return material;
        }

        rhi::ShaderModule* vert_shader = vert_shader_shared.get();
        rhi::ShaderModule* frag_shader = frag_shader_shared.get();

        rhi::GraphicsPipeline* pipeline        = nullptr;
        rhi::PipelineLayout*   pipeline_layout = nullptr;

        std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;

        const std::size_t settings_hash = settings.hash() ^
            (static_cast<std::size_t>(loader.swapchain_ ? loader.swapchain_->sample_count() : rhi::TextureSampleCount::Count1) << 20);
        const auto cached = resource_cache->get_cached_pipeline(vertex_shader_id, fragment_shader_id, settings_hash);

        if (cached)
        {
            pipeline               = cached->pipeline.get();
            pipeline_layout        = cached->layout.get();
            descriptor_set_layouts.reserve(cached->descriptor_set_layouts.size());
            for (const auto& layout : cached->descriptor_set_layouts)
                descriptor_set_layouts.push_back(layout.get());
        }
        else
        {
            std::vector<rhi::ShaderModule*> pipeline_shaders{ vert_shader, frag_shader };

            rhi::PipelineBuilder builder(api, device, pipeline_shaders);

            if (!builder.build_descriptor_set_layouts())
            {
                Log::error("Failed to build descriptor set layouts for material");
                return material;
            }

            auto pipeline_layout_owner = builder.build_pipeline_layout();
            if (!pipeline_layout_owner)
            {
                Log::error("Failed to create pipeline layout for material");
                return material;
            }

            const auto* swapchain = loader.swapchain_;
            if (!swapchain)
            {
                Log::error("Swapchain not available in MaterialLoader. Cannot create material.");
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

            auto pipeline_owner = builder.build_graphics_pipeline(
                pipeline_layout_owner.get(),
                swapchain,
                swapchain->depth_format(),
                raster_state,
                depth_state,
                {},
                {},
                rhi::PrimitiveTopology::TriangleList);

            if (!pipeline_owner)
            {
                Log::error("Failed to create graphics pipeline for material");
                return material;
            }

            auto built_descriptor_set_layouts = builder.take_descriptor_set_layouts();
            rhi::ResourceCache::CachedGraphicsPipeline cached_pipeline;
            cached_pipeline.pipeline = std::move(pipeline_owner);
            cached_pipeline.layout = std::move(pipeline_layout_owner);

            pipeline = cached_pipeline.pipeline.get();
            pipeline_layout = cached_pipeline.layout.get();

            descriptor_set_layouts.reserve(built_descriptor_set_layouts.size());
            cached_pipeline.descriptor_set_layouts.reserve(built_descriptor_set_layouts.size());
            for (auto& layout : built_descriptor_set_layouts)
            {
                descriptor_set_layouts.push_back(layout.get());
                cached_pipeline.descriptor_set_layouts.emplace_back(layout.release());
            }

            const auto stored_cached = resource_cache->cache_graphics_pipeline(
                vertex_shader_id,
                fragment_shader_id,
                settings_hash,
                std::move(cached_pipeline));

            pipeline = stored_cached ? stored_cached->pipeline.get() : nullptr;
            pipeline_layout = stored_cached ? stored_cached->layout.get() : nullptr;

            descriptor_set_layouts.clear();
            if (stored_cached)
            {
                descriptor_set_layouts.reserve(stored_cached->descriptor_set_layouts.size());
                for (const auto& layout : stored_cached->descriptor_set_layouts)
                {
                    descriptor_set_layouts.push_back(layout.get());
                }
            }
        }

        const std::uint32_t frame_count = std::max<std::uint32_t>(
            loader.swapchain_ ? loader.swapchain_->max_frames_in_flight() : 1u,
            1u);

        std::vector<std::vector<rhi::DescriptorSet*>> descriptor_sets_per_frame(frame_count);
        std::vector<rhi::DescriptorSet*> allocated_descriptor_sets;
        allocated_descriptor_sets.reserve(frame_count * descriptor_set_layouts.size());

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

                    if (!allocated_descriptor_sets.empty())
                    {
                        descriptor_pool->free_descriptor_sets(allocated_descriptor_sets);
                    }

                    return material;
                }

                descriptor_sets.push_back(desc_set);
                allocated_descriptor_sets.push_back(desc_set);
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
        std::vector<rhi::ShaderModule*> reflection_shaders{ vert_shader, frag_shader };
        reflection->build_from_shaders(reflection_shaders);
        material.reflection_.reset(reflection);

        std::uint32_t push_constant_bytes = 0;
        for (const auto& range : reflection->push_constant_ranges() | std::views::values)
        {
            push_constant_bytes = std::max(push_constant_bytes, range.offset + range.size);
        }

        material.push_constant_staging_.assign(push_constant_bytes, std::uint8_t{ 0 });

        auto bind_default_texture = [&](const std::string& binding_name, const std::string_view texture_name)
        {
            const auto info = material.lookup_binding(binding_name);
            if (!info.has_value() ||
                info->descriptor_type != static_cast<std::uint32_t>(rhi::DescriptorType::CombinedImageSampler))
            {
                return;
            }

            if (Texture* texture = Texture::try_get(texture_name))
                material.update_texture(binding_name, *texture, SamplerLoader::instance().default_sampler());
        };

        bind_default_texture("albedo_map", "__boza_white");
        bind_default_texture("normal_map", "__boza_flat_normal");
        bind_default_texture("roughness_map", "__boza_white");
        bind_default_texture("metallic_map", "__boza_black");
        bind_default_texture("ao_map", "__boza_white");
        bind_default_texture("ssao_texture", "__boza_white");

        return material;
    }

    void MaterialLoader::update_time_ubo(const float time, const float delta_time)
    {
        if (!time_ubo_) return;

        const TimeUBO time_data{
            .time = time,
            .delta_time = delta_time,
            .padding = { 0.0f, 0.0f }
        };

        time_ubo_->upload(&time_data, sizeof(TimeUBO), 0);
    }

    void MaterialLoader::begin_light_update(
        const glm::vec3& view_position,
        const glm::mat4& view,
        const glm::mat4& projection,
        const std::uint32_t screen_width,
        const std::uint32_t screen_height,
        const float near_clip,
        const float far_clip)
    {
        point_lights_.clear();
        directional_lights_.clear();
        spot_lights_.clear();
        shadow_casters_.clear();
        point_shadow_sources_.clear();
        point_shadow_view_projections_.clear();
        spot_shadow_sources_.clear();
        spot_shadow_view_projections_.clear();

        light_view_ = view;
        light_projection_ = projection;

        lighting_data_ = LightingUBO{
            .view_pos = glm::vec4{ view_position, 1.0f },
            .ambient_color = glm::vec4{ 1.0f, 0.96f, 0.88f, 0.16f },
            .light_counts = glm::uvec4{ 0u },
            .cluster_grid = glm::uvec4{ 0u },
            .screen_params = glm::vec4{
                static_cast<float>(std::max(screen_width, 1u)),
                static_cast<float>(std::max(screen_height, 1u)),
                std::max(near_clip, 0.001f),
                std::max(far_clip, near_clip + 1.0f)
            },
            .shadow_params = glm::vec4{ 0.0012f, 0.00005f, static_cast<float>(directional_shadow_map_size_), 0.0f },
            .directional_shadow_splits = glm::vec4{ 0.0f },
            .directional_shadow_view_projections = {
                glm::identity<glm::mat4>(),
                glm::identity<glm::mat4>(),
                glm::identity<glm::mat4>(),
                glm::identity<glm::mat4>()
            }
        };

        directional_shadow_enabled_ = false;
        point_shadow_enabled_ = false;
        spot_shadow_enabled_ = false;

        cluster_grid_size_ = compute_cluster_grid_size(screen_width, screen_height);
        ensure_cluster_buffers(cluster_grid_size_);
        lighting_data_.cluster_grid = glm::uvec4{ cluster_grid_size_, cluster_count(cluster_grid_size_) };
    }

    void MaterialLoader::add_point_light(const glm::vec3& position, const PointLight& light)
    {
        if (point_lights_.size() >= max_point_lights_) return;

        const float range = safe_range(light.range, 10.0f);
        const bool casts_point_shadow =
            light.casts_shadows &&
            point_shadow_map_ &&
            point_shadow_sources_.size() < max_point_shadow_maps_;
        const std::uint32_t shadow_slot = static_cast<std::uint32_t>(point_shadow_sources_.size());

        if (casts_point_shadow)
        {
            point_shadow_sources_.push_back(PointShadowSource{
                .slot = shadow_slot,
                .position = position,
                .range = range
            });
        }

        point_lights_.push_back(GpuPointLight{
            .position_range = glm::vec4{ position, range },
            .color_intensity = glm::vec4{
                glm::max(light.color, glm::vec3{ 0.0f }),
                std::max(light.intensity, 0.0f)
            },
            .shadow_data = glm::vec4{
                casts_point_shadow ? 1.0f : 0.0f,
                glm::clamp(light.shadow_strength, 0.0f, 1.0f),
                static_cast<float>(shadow_slot),
                0.0f
            }
        });
    }

    void MaterialLoader::add_directional_light(const glm::vec3& direction, const DirectionalLight& light)
    {
        if (directional_lights_.size() >= max_directional_lights_) return;

        const bool shadow_slot_available = std::ranges::none_of(
            directional_lights_,
            [](const GpuDirectionalLight& existing)
            {
                return existing.direction_shadow.w > 0.0f;
            });

        static bool warned_about_multiple_directional_shadows = false;
        if (light.casts_shadows && !shadow_slot_available && !warned_about_multiple_directional_shadows)
        {
            Log::warn("Only one shadowed directional light is supported; additional directional shadows are disabled");
            warned_about_multiple_directional_shadows = true;
        }

        directional_lights_.push_back(GpuDirectionalLight{
            .direction_shadow = glm::vec4{
                safe_direction(direction),
                light.casts_shadows && shadow_slot_available
                    ? glm::clamp(light.shadow_strength, 0.0f, 1.0f)
                    : 0.0f
            },
            .color_intensity = glm::vec4{
                glm::max(light.color, glm::vec3{ 0.0f }),
                std::max(light.intensity, 0.0f)
            }
        });
    }

    void MaterialLoader::add_spot_light(
        const glm::vec3& position,
        const glm::vec3& direction,
        const SpotLight& light)
    {
        if (spot_lights_.size() >= max_spot_lights_) return;

        const glm::vec3 normalized_direction = safe_direction(direction);
        const float range = safe_range(light.range, 12.0f);
        const float inner_cos = std::cos(std::min(light.inner_angle, light.outer_angle));
        const float outer_cos = std::cos(std::max(light.inner_angle, light.outer_angle));
        const bool casts_spot_shadow =
            light.casts_shadows &&
            spot_shadow_map_ &&
            spot_shadow_sources_.size() < max_spot_shadow_maps_;
        const std::uint32_t shadow_slot = static_cast<std::uint32_t>(spot_shadow_sources_.size());

        if (casts_spot_shadow)
        {
            spot_shadow_sources_.push_back(SpotShadowSource{
                .slot = shadow_slot,
                .position = position,
                .direction = normalized_direction,
                .outer_angle = std::max(light.outer_angle, light.inner_angle),
                .range = range
            });
        }

        spot_lights_.push_back(GpuSpotLight{
            .position_range = glm::vec4{ position, range },
            .direction_angles = glm::vec4{ normalized_direction, outer_cos },
            .color_intensity = glm::vec4{
                glm::max(light.color, glm::vec3{ 0.0f }),
                std::max(light.intensity, 0.0f)
            },
            .shadow_data = glm::vec4{
                casts_spot_shadow ? 1.0f : 0.0f,
                glm::clamp(light.shadow_strength, 0.0f, 1.0f),
                inner_cos,
                static_cast<float>(shadow_slot)
            }
        });
    }

    void MaterialLoader::add_shadow_caster(
        const glm::vec3& center,
        const glm::vec3& axis_x,
        const glm::vec3& axis_y,
        const glm::vec3& axis_z,
        const glm::vec3& half_extents)
    {
        if (shadow_casters_.size() >= max_shadow_casters_) return;

        const glm::vec3 extents = glm::max(half_extents, glm::vec3{ 0.001f });

        shadow_casters_.push_back(GpuShadowCaster{
            .center_half_x = glm::vec4{ center, extents.x },
            .axis_x_half_y = glm::vec4{ axis_x, extents.y },
            .axis_y_half_z = glm::vec4{ axis_y, extents.z },
            .axis_z_padding = glm::vec4{ axis_z, 0.0f }
        });
    }

    void MaterialLoader::upload_light_buffers()
    {
        const std::uint32_t total_clusters = cluster_count(cluster_grid_size_);
        cluster_records_.assign(total_clusters, GpuClusterRecord{});
        cluster_light_indices_.clear();

        if (cluster_light_counter_buffer_)
            cluster_light_counter_buffer_->upload(GpuClusterLightCounter{});

        lighting_data_.light_counts = glm::uvec4{
            static_cast<std::uint32_t>(point_lights_.size()),
            static_cast<std::uint32_t>(directional_lights_.size()),
            static_cast<std::uint32_t>(spot_lights_.size()),
            static_cast<std::uint32_t>(shadow_casters_.size())
        };

        directional_shadow_enabled_ = false;
        lighting_data_.directional_shadow_splits = glm::vec4{ lighting_data_.screen_params.w };
        for (glm::mat4& matrix : lighting_data_.directional_shadow_view_projections)
            matrix = glm::identity<glm::mat4>();
        lighting_data_.shadow_params.w = 0.0f;

        if (directional_shadow_map_)
        {
            const float near_clip = std::max(lighting_data_.screen_params.z, 0.001f);
            const float far_clip = std::max(lighting_data_.screen_params.w, near_clip + 1.0f);
            const auto splits = build_directional_shadow_splits(near_clip, far_clip);

            lighting_data_.directional_shadow_splits = glm::vec4{
                splits[0],
                splits[1],
                splits[2],
                splits[3]
            };

            for (const GpuDirectionalLight& light : directional_lights_)
            {
                if (light.direction_shadow.w <= 0.0f) continue;

                float cascade_near = near_clip;
                bool built_any_cascade = false;

                constexpr std::array cascade_extents{
                    56.0f,
                    96.0f,
                    160.0f,
                    directional_shadow_extent_
                };

                for (std::uint32_t cascade_index = 0; cascade_index < directional_shadow_cascade_count_; ++cascade_index)
                {
                    const float cascade_far = splits[cascade_index];
                    const float cascade_extent = std::max(cascade_extents[cascade_index], cascade_far);

                    const glm::mat4 shadow_view_projection = build_directional_shadow_view_projection(
                        light_view_,
                        light_projection_,
                        cascade_near,
                        cascade_far,
                        glm::vec3{ light.direction_shadow },
                        directional_shadow_map_size_,
                        cascade_extent);

                    lighting_data_.directional_shadow_view_projections[cascade_index] = shadow_view_projection;
                    built_any_cascade = true;

                    cascade_near = cascade_far;
                }

                if (built_any_cascade)
                {
                    lighting_data_.shadow_params.z = static_cast<float>(directional_shadow_map_size_);
                    lighting_data_.shadow_params.w = 1.0f;
                    directional_shadow_enabled_ = true;
                }

                break;
            }
        }

        point_shadow_enabled_ = point_shadow_map_ && !point_shadow_sources_.empty();
        point_shadow_view_projections_.clear();

        if (point_shadow_enabled_)
        {
            point_shadow_view_projections_.reserve(point_shadow_sources_.size() * 6u);

            for (const PointShadowSource& source : point_shadow_sources_)
            {
                const auto matrices = build_point_shadow_view_projections(source.position, source.range);
                point_shadow_view_projections_.insert(
                    point_shadow_view_projections_.end(),
                    matrices.begin(),
                    matrices.end());
            }
        }
        else
        {
            for (GpuPointLight& light : point_lights_)
                light.shadow_data.x = 0.0f;
        }

        spot_shadow_enabled_ = spot_shadow_map_ && !spot_shadow_sources_.empty();
        spot_shadow_view_projections_.clear();

        if (spot_shadow_enabled_)
        {
            spot_shadow_view_projections_.reserve(spot_shadow_sources_.size());

            for (const SpotShadowSource& source : spot_shadow_sources_)
            {
                spot_shadow_view_projections_.push_back(build_spot_shadow_view_projection(
                    source.position,
                    source.direction,
                    source.outer_angle,
                    source.range));
            }
        }
        else
        {
            for (GpuSpotLight& light : spot_lights_)
                light.shadow_data.x = 0.0f;
        }

        static std::uint32_t light_log_frame = 0;
        if (light_log_frame < 4 || light_log_frame % 5000 == 0)
        {
            Log::debug(
                "upload_light_buffers frame {}: points={} directionals={} spots={} shadow_casters={} directional_shadow={} point_shadows={} spot_shadows={} cluster_grid={}x{}x{} cluster_capacity={} cluster_index_capacity={} cluster_indices={}",
                light_log_frame,
                point_lights_.size(),
                directional_lights_.size(),
                spot_lights_.size(),
                shadow_casters_.size(),
                directional_shadow_enabled_,
                point_shadow_sources_.size(),
                spot_shadow_sources_.size(),
                cluster_grid_size_.x,
                cluster_grid_size_.y,
                cluster_grid_size_.z,
                cluster_record_capacity_,
                cluster_light_index_capacity_,
                cluster_light_indices_.size());
        }
        ++light_log_frame;

        if (lighting_ubo_) lighting_ubo_->upload(lighting_data_);
        if (point_lights_buffer_ && !point_lights_.empty()) point_lights_buffer_->upload(std::span{ point_lights_ });
        if (directional_lights_buffer_ && !directional_lights_.empty()) directional_lights_buffer_->upload(std::span{ directional_lights_ });
        if (spot_lights_buffer_ && !spot_lights_.empty()) spot_lights_buffer_->upload(std::span{ spot_lights_ });
        if (cluster_records_buffer_ && !cluster_records_.empty()) cluster_records_buffer_->upload(std::span{ cluster_records_ });
        if (cluster_light_indices_buffer_ && !cluster_light_indices_.empty())
            cluster_light_indices_buffer_->upload(std::span{ cluster_light_indices_ });
        if (shadow_casters_buffer_ && !shadow_casters_.empty()) shadow_casters_buffer_->upload(std::span{ shadow_casters_ });
        if (point_shadow_matrices_buffer_ && !point_shadow_view_projections_.empty())
            point_shadow_matrices_buffer_->upload(std::span{ point_shadow_view_projections_ });
        if (spot_shadow_matrices_buffer_ && !spot_shadow_view_projections_.empty())
            spot_shadow_matrices_buffer_->upload(std::span{ spot_shadow_view_projections_ });
    }
}
