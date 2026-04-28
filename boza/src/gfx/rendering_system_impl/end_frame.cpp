module boza.gfx;

import :buffer;
import :compute_dispatcher;
import :material;
import :rendering_system;
import :rendering_system_common;
import :texture;

import <flecs.h>;
import boza.core;
import boza.rhi;
import boza.rhi.render_context;
import boza.gfx.material_loader;

namespace boza
{
    void RenderingSystem::bind_shadow_camera(Material& material, const glm::mat4& light_view_projection)
    {
        gfx::MaterialLoader::instance().bind_engine_resources(&material);

        if (!ensure_shadow_camera_buffer()) return;

        shadow_camera_buffer_->upload(gfx::CameraUBO{
            .view = glm::identity<glm::mat4>(),
            .proj = light_view_projection
        });

        material.update_buffer("cameraUBO", *shadow_camera_buffer_);
    }

    void RenderingSystem::collect_shadow_models(
        const Mesh& mesh,
        const MeshBucket& bucket,
        const FrustumState& shadow_frustum)
    {
        shadow_instance_models_.clear();
        shadow_instance_models_.reserve(bucket.elements.size());

        for (const RenderElement& elem : bucket.elements)
        {
            if (!elem.entity.valid() || !elem.entity.active) continue;
            const auto* caster = std::as_const(elem.entity).try_get_component<ShadowCaster>();
            if (!caster) continue;

            const auto* transform = std::as_const(elem.entity).try_get_component<Transform>();
            if (!transform) continue;

            ShadowCasterGeometry geometry{};
            if (!build_shadow_caster_geometry(*transform, *caster, &mesh, geometry)) continue;
            if (!shadow_frustum.sphere_visible(geometry.center, geometry.sphere.w)) continue;

            shadow_instance_models_.push_back(transform->world_matrix());
        }
    }

    void RenderingSystem::render_shadow_bucket(
        rhi::CommandBuffer* cmd,
        Material* material,
        const Mesh& mesh,
        const std::span<const glm::mat4> models,
        void* render_info_opaque,
        const glm::mat4& light_view_projection,
        const bool shadow_mode)
    {
        auto* render_info = static_cast<MaterialRenderInfo*>(render_info_opaque);
        if (models.empty()) return;

        auto* gpu_mesh = get_or_create_gpu_mesh(const_cast<Mesh*>(&mesh));
        if (!gpu_mesh) return;

        auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->vertex_buffer));
        auto* index_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->index_buffer));
        if (!vertex_buffer_rhi || !index_buffer_rhi) return;

        cmd->bind_vertex_buffer(vertex_buffer_rhi);
        cmd->bind_index_buffer(index_buffer_rhi);

        MaterialAccess::bind_descriptor_sets(*material);

        for (const glm::mat4& model_matrix : models)
        {
            push_ranges(cmd, *material, render_info, &model_matrix, false, &light_view_projection, shadow_mode);
            cmd->draw_indexed(gpu_mesh->index_count);
        }
    }

    void RenderingSystem::execute_shadow_caster_cull_pass()
    {
        clear_shadow_cull_results();
    }

    void RenderingSystem::execute_depth_prepass()
    {
        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!cmd) return;

        auto& material_loader = gfx::MaterialLoader::instance();
        Texture* frame_depth_texture = material_loader.frame_depth_texture();
        if (!frame_depth_texture) return;

        auto* frame_depth_handle = static_cast<rhi::Texture*>(TextureAccess::handle(*frame_depth_texture));
        if (!frame_depth_handle) return;

        const TextureLayout current_depth_layout = TextureAccess::layout(*frame_depth_texture);
        if (current_depth_layout == TextureLayout::ShaderReadOnly)
        {
            cmd->image_barrier(
                frame_depth_handle,
                rhi::ResourceState::ShaderResource,
                rhi::ResourceState::DepthStencil);
            TextureAccess::set_layout(*frame_depth_texture, TextureLayout::DepthStencilAttachment);
        }
        else if (current_depth_layout == TextureLayout::Undefined)
        {
            cmd->image_barrier(
                frame_depth_handle,
                rhi::ResourceState::Undefined,
                rhi::ResourceState::DepthStencil);
            TextureAccess::set_layout(*frame_depth_texture, TextureLayout::DepthStencilAttachment);
        }

        cmd->begin_depth_rendering(
            frame_depth_handle,
            frame_depth_texture->width,
            frame_depth_texture->height,
            1.0f,
            true);

        for (auto& [material, mat_group] : render_cache_)
        {
            if (!material || material->shadow_only()) continue;

            const MaterialSettings& material_settings = MaterialAccess::settings(*material);
            if (material_settings.cull_mode == CullMode::None) continue;

            if (!ensure_shadow_pipeline(material)) continue;

            MaterialRenderInfo* render_info = get_or_build_render_info(material);

            auto shadow_pipeline_it = shadow_pipelines_.find(material);
            if (shadow_pipeline_it == shadow_pipelines_.end()) continue;

            cmd->bind_graphics_pipeline(shadow_pipeline_it->second.pipeline.get());

            for (auto& [mesh, bucket] : mat_group.mesh_buckets)
            {
                shadow_instance_models_.clear();
                shadow_instance_models_.reserve(bucket.num_visible);

                for (const RenderElement& elem : bucket.elements)
                {
                    if (!elem.visible) continue;

                    const auto* transform = std::as_const(elem.entity).try_get_component<Transform>();
                    if (!transform) continue;

                    shadow_instance_models_.push_back(transform->world_matrix());
                }

                render_shadow_bucket(
                    cmd,
                    material,
                    *mesh,
                    std::span<const glm::mat4>{ shadow_instance_models_.data(), shadow_instance_models_.size() },
                    render_info,
                    glm::identity<glm::mat4>(),
                    false);
            }
        }

        cmd->end_rendering();
        cmd->image_barrier(
            frame_depth_handle,
            rhi::ResourceState::DepthStencil,
            rhi::ResourceState::ShaderResource);
        TextureAccess::set_layout(*frame_depth_texture, TextureLayout::ShaderReadOnly);
    }

    void RenderingSystem::render_shadow_pass(
        Texture* shadow_map,
        const std::span<const glm::mat4> matrices,
        bool& layout_initialized,
        const std::string_view log_label,
        [[maybe_unused]] const std::uint32_t dispatcher_pass_index)
    {
        assert_render_thread();

        auto* shadow_texture = shadow_map
            ? static_cast<rhi::Texture*>(TextureAccess::handle(*shadow_map))
            : nullptr;

        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!shadow_map || !shadow_texture || matrices.empty() || !cmd) return;

        const std::uint32_t shadow_width = shadow_map->width;
        const std::uint32_t shadow_height = shadow_map->height;

        cmd->image_barrier(
            shadow_texture,
            rhi::ResourceState::ShaderResource,
            rhi::ResourceState::DepthStencil);
        layout_initialized = true;

        std::uint32_t shadow_draws = 0;
        bool previous_layer_used_gpu_shadow_work = false;
        ComputeDispatcher* gpu_driven_shadow_dispatcher = get_frame_dispatcher(
            gpu_driven_shadow_cull_dispatchers_,
            "gpu_frustum_cull");

        for (std::uint32_t layer = 0; layer < matrices.size(); ++layer)
        {
            if (previous_layer_used_gpu_shadow_work)
                cmd->draw_to_compute_barrier();

            const glm::mat4& light_view_projection = matrices[layer];
            FrustumState shadow_frustum{};
            shadow_frustum.set_from_view_projection(light_view_projection);

            std::array<glm::vec4, 6> shadow_frustum_planes{};
            for (std::size_t i = 0; i < shadow_frustum.planes.size(); ++i)
            {
                shadow_frustum_planes[i] = glm::vec4{
                    shadow_frustum.planes[i].normal,
                    shadow_frustum.planes[i].distance
                };
            }

            bool has_gpu_shadow_work = false;

            for (auto& [material, mat_group] : render_cache_)
            {
                if (!material || !ensure_shadow_pipeline(material)) continue;

                MaterialRenderInfo* render_info = get_or_build_render_info(material);
                const bool supports_shadow_instancing =
                    gpu_shadow_instancing_enabled_ &&
                    render_info &&
                    render_info->instancing_range_index.has_value() &&
                    *render_info->instancing_range_index < render_info->ranges.size() &&
                    supports_gpu_culled_instancing(render_info->ranges[*render_info->instancing_range_index]);

                if (!supports_shadow_instancing) continue;

                for (auto& [mesh, bucket] : mat_group.mesh_buckets)
                {
                    if (bucket.elements.size() < instancing_threshold_) continue;

                    if (!upload_mesh_bucket_candidates(bucket, mesh, material)) continue;
                    if (!bucket.shadow_candidate_buffer || bucket.shadow_candidate_count == 0u) continue;

                    auto* gpu_mesh = get_or_create_gpu_mesh(mesh);
                    if (!gpu_mesh) continue;

                    ShadowCullBufferState* state = get_shadow_gpu_cull_buffer_state(material, mesh);
                    if (!state) continue;

                    if (!ensure_generic_shadow_gpu_cull_capacity(*state, bucket.shadow_candidate_count)) continue;
                    if (!state->params_buffer || !state->culled_instance_buffer || !state->indirect_buffer) continue;

                    has_gpu_shadow_work =
                        dispatch_gpu_driven_cull(
                            *bucket.shadow_candidate_buffer,
                            bucket.shadow_candidate_count,
                            *gpu_mesh,
                            *state->params_buffer,
                            *state->culled_instance_buffer,
                            *state->indirect_buffer,
                            shadow_frustum_planes,
                            *gpu_driven_shadow_dispatcher) || has_gpu_shadow_work;
                }
            }

            if (gpu_driven_shadow_dispatcher && !gpu_driven_shadow_dispatcher->failed())
            {
                Scene::world().each(
                    [&](const flecs::entity entity, MeshRenderer& mr, GpuDrivenInstances& gpu_instances)
                {
                    if (!entity.is_valid() || !entity.enabled()) return;
                    if (!gpu_instances.casts_shadows) return;
                    if (entity.has<tags::ShadowOnly>()) return;

                    Material* material = mr.material;
                    Mesh* mesh = mr.mesh;
                    const Buffer* candidates = gpu_instances.shadow_candidates();
                    const std::uint32_t candidate_count = gpu_instances.shadow_count();

                    if (!material || !mesh || !candidates || candidate_count == 0u) return;
                    if (!ensure_shadow_pipeline(material)) return;

                    MaterialRenderInfo* render_info = get_or_build_render_info(material);
                    if (!render_info || !render_info->instancing_range_index.has_value()) return;

                    const std::size_t range_index = *render_info->instancing_range_index;
                    if (range_index >= render_info->ranges.size()) return;

                    const PushConstantRangeRuntime& range = render_info->ranges[range_index];
                    if (!supports_gpu_culled_instancing(range)) return;

                    GpuMesh* gpu_mesh = get_or_create_gpu_mesh(mesh);
                    if (!gpu_mesh) return;

                    GpuDrivenBatchState* state = get_gpu_driven_batch_state(entity.id());
                    if (!state) return;

                    const std::size_t required_visible_bytes =
                        static_cast<std::size_t>(candidate_count) * static_cast<std::size_t>(range.instancing_stride);

                    if (!ensure_gpu_driven_batch_capacity(*state, required_visible_bytes)) return;

                    has_gpu_shadow_work =
                        dispatch_gpu_driven_cull(
                            *candidates,
                            candidate_count,
                            *gpu_mesh,
                            *state->params_buffer,
                            *state->visible_buffer,
                            *state->indirect_buffer,
                            shadow_frustum_planes,
                            *gpu_driven_shadow_dispatcher) || has_gpu_shadow_work;
                });
            }

            if (has_gpu_shadow_work)
                cmd->compute_to_draw_barrier();

            cmd->begin_depth_rendering_layer(shadow_texture, layer, shadow_width, shadow_height, 1.0f);

            for (auto& [material, mat_group] : render_cache_)
            {
                if (!material || !ensure_shadow_pipeline(material)) continue;

                MaterialRenderInfo* render_info = get_or_build_render_info(material);
                auto shadow_pipeline_it = shadow_pipelines_.find(material);
                if (shadow_pipeline_it == shadow_pipelines_.end()) continue;

                cmd->bind_graphics_pipeline(shadow_pipeline_it->second.pipeline.get());

                for (auto& [mesh, bucket] : mat_group.mesh_buckets)
                {
                    collect_shadow_models(*mesh, bucket, shadow_frustum);
                    render_shadow_bucket(
                        cmd,
                        material,
                        *mesh,
                        std::span<const glm::mat4>{ shadow_instance_models_.data(), shadow_instance_models_.size() },
                        render_info,
                        light_view_projection,
                        true);

                    shadow_draws += static_cast<std::uint32_t>(shadow_instance_models_.size());
                }
            }

            Scene::world().each(
                [&](const flecs::entity entity, MeshRenderer& mr, GpuDrivenInstances& gpu_instances)
            {
                if (!entity.is_valid() || !entity.enabled()) return;
                if (!gpu_instances.casts_shadows) return;

                Material* material = mr.material;
                Mesh* mesh = mr.mesh;
                const std::uint32_t candidate_count = gpu_instances.shadow_count();

                if (!material || !mesh || candidate_count == 0u) return;
                if (!ensure_shadow_pipeline(material)) return;

                MaterialRenderInfo* render_info = get_or_build_render_info(material);
                if (!render_info || !render_info->instancing_range_index.has_value()) return;

                const std::size_t range_index = *render_info->instancing_range_index;
                if (range_index >= render_info->ranges.size()) return;

                const PushConstantRangeRuntime& range = render_info->ranges[range_index];

                auto shadow_pipeline_it = shadow_pipelines_.find(material);
                if (shadow_pipeline_it == shadow_pipelines_.end()) return;

                GpuMesh* gpu_mesh = get_or_create_gpu_mesh(mesh);
                if (!gpu_mesh) return;

                auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->vertex_buffer));
                auto* index_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->index_buffer));
                if (!vertex_buffer_rhi || !index_buffer_rhi) return;

                if (entity.has<tags::ShadowOnly>())
                {
                    const Buffer* candidates = gpu_instances.shadow_candidates();
                    if (!candidates) return;

                    cmd->bind_graphics_pipeline(shadow_pipeline_it->second.pipeline.get());
                    cmd->bind_vertex_buffer(vertex_buffer_rhi);
                    cmd->bind_index_buffer(index_buffer_rhi);
                    material->update_buffer(range.instancing_ssbo_name, *candidates);
                    MaterialAccess::bind_descriptor_sets(*material);
                    push_ranges(cmd, *material, render_info, nullptr, true, &light_view_projection, true);
                    cmd->draw_indexed(gpu_mesh->index_count, candidate_count, 0, 0, 0);
                    ++shadow_draws;
                    return;
                }

                if (!supports_gpu_culled_instancing(range)) return;

                GpuDrivenBatchState* state = get_gpu_driven_batch_state(entity.id());
                if (!state || !state->visible_buffer || !state->indirect_buffer) return;

                auto* indirect_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(*state->indirect_buffer));
                if (!vertex_buffer_rhi || !index_buffer_rhi || !indirect_buffer_rhi) return;

                cmd->bind_graphics_pipeline(shadow_pipeline_it->second.pipeline.get());
                cmd->bind_vertex_buffer(vertex_buffer_rhi);
                cmd->bind_index_buffer(index_buffer_rhi);

                material->update_buffer(range.instancing_ssbo_name, *state->visible_buffer);
                MaterialAccess::bind_descriptor_sets(*material);
                push_ranges(cmd, *material, render_info, nullptr, true, &light_view_projection, true);
                cmd->draw_indexed_indirect(
                    indirect_buffer_rhi,
                    0,
                    1,
                    sizeof(GpuIndexedDrawCommand));
                ++shadow_draws;
            });

            cmd->end_rendering();
            previous_layer_used_gpu_shadow_work = has_gpu_shadow_work;
        }

        cmd->image_barrier(
            shadow_texture,
            rhi::ResourceState::DepthStencil,
            rhi::ResourceState::ShaderResource);

        static flat_map<std::string, std::uint32_t> log_frames{};
        std::uint32_t& log_frame = log_frames[std::string{ log_label }];
        if (log_frame < 4 || log_frame % 5000 == 0)
        {
            Log::debug(
                "{} frame {}: layers={} draws={} size={}x{}",
                log_label,
                log_frame,
                matrices.size(),
                shadow_draws,
                shadow_width,
                shadow_height);
        }
        ++log_frame;
    }

    void RenderingSystem::execute_directional_shadow_pass()
    {
        auto& material_loader = gfx::MaterialLoader::instance();
        if (!material_loader.directional_shadow_enabled()) return;

        Texture* shadow_map = material_loader.directional_shadow_map();
        if (!shadow_map) return;

        render_shadow_pass(
            shadow_map,
            std::span<const glm::mat4>{
                material_loader.directional_shadow_view_projections().data(),
                material_loader.directional_shadow_view_projections().size()
            },
            directional_shadow_map_layout_initialized_,
            "render_directional_shadow_map",
            0u);
    }

    void RenderingSystem::execute_point_shadow_pass()
    {
        auto& material_loader = gfx::MaterialLoader::instance();
        if (!material_loader.point_shadow_enabled()) return;

        Texture* shadow_map = material_loader.point_shadow_map();
        if (!shadow_map) return;

        render_shadow_pass(
            shadow_map,
            material_loader.point_shadow_view_projections(),
            point_shadow_map_layout_initialized_,
            "render_point_shadow_maps",
            1u);
    }

    void RenderingSystem::execute_spot_shadow_pass()
    {
        auto& material_loader = gfx::MaterialLoader::instance();
        if (!material_loader.spot_shadow_enabled()) return;

        Texture* shadow_map = material_loader.spot_shadow_map();
        if (!shadow_map) return;

        render_shadow_pass(
            shadow_map,
            material_loader.spot_shadow_view_projections(),
            spot_shadow_map_layout_initialized_,
            "render_spot_shadow_maps",
            2u);
    }

    void RenderingSystem::prepare_frame_material_bindings()
    {
        if (pipeline_materials_dirty_)
        {
            rebuild_pipeline_materials();
            pipeline_materials_dirty_ = false;
        }

        auto& material_loader = gfx::MaterialLoader::instance();

        for (auto& [material, mat_group] : render_cache_)
        {
            if (!material || mat_group.num_visible == 0) continue;

            MaterialRenderInfo* render_info = get_or_build_render_info(material);
            ensure_fallback_ssbo_bound(*material, render_info);
            material_loader.bind_engine_resources(material);
            bind_frame_render_resources(*material);
        }

        Scene::world().each(
            [&](const flecs::entity entity, MeshRenderer& mr, GpuDrivenInstances&)
        {
            if (!entity.is_valid() || !entity.enabled()) return;

            Material* material = mr.material;
            if (!material) return;

            MaterialRenderInfo* render_info = get_or_build_render_info(material);
            ensure_fallback_ssbo_bound(*material, render_info);
            material_loader.bind_engine_resources(material);
            bind_frame_render_resources(*material);
        });
    }

    void RenderingSystem::execute_gpu_instance_culling_pass()
    {
        if (!gpu_indirect_instancing_enabled_ || !frustum_.valid) return;

        for (auto& [material, mat_group] : render_cache_)
        {
            MaterialRenderInfo* render_info = get_or_build_render_info(material);
            if (!render_info || !render_info->instancing_range_index.has_value()) continue;

            const std::size_t range_index = *render_info->instancing_range_index;
            if (range_index >= render_info->ranges.size()) continue;

            const PushConstantRangeRuntime& range = render_info->ranges[range_index];
            if (!supports_gpu_culled_instancing(range)) continue;

            for (auto& [mesh, bucket] : mat_group.mesh_buckets)
            {
                if (bucket.num_visible < instancing_threshold_) continue;

                if (!upload_mesh_bucket_candidates(bucket, mesh, material)) continue;
                if (!bucket.candidate_buffer || bucket.candidate_count == 0u) continue;

                GpuMesh* gpu_mesh = get_or_create_gpu_mesh(mesh);
                if (!gpu_mesh) continue;

                GpuCullBufferState* state = get_gpu_cull_buffer_state(material, mesh);
                if (!state) continue;

                if (!ensure_generic_gpu_cull_capacity(*state, bucket.candidate_count)) continue;
                if (!state->params_buffer || !state->culled_instance_buffer || !state->indirect_buffer) continue;

                ComputeDispatcher* dispatcher = get_frame_dispatcher(state->dispatchers, "gpu_frustum_cull");
                if (!dispatcher || dispatcher->failed()) continue;

                (void)dispatch_gpu_driven_cull(
                    *bucket.candidate_buffer,
                    bucket.candidate_count,
                    *gpu_mesh,
                    *state->params_buffer,
                    *state->culled_instance_buffer,
                    *state->indirect_buffer,
                    gpu_cull_frustum_planes_,
                    *dispatcher);
            }
        }

        ComputeDispatcher* gpu_driven_forward_dispatcher = get_frame_dispatcher(
            gpu_driven_forward_cull_dispatchers_,
            "gpu_frustum_cull");

        if (!gpu_driven_forward_dispatcher || gpu_driven_forward_dispatcher->failed()) return;

        Scene::world().each(
            [&](const flecs::entity entity, MeshRenderer& mr, GpuDrivenInstances& gpu_instances)
        {
            if (!entity.is_valid() || !entity.enabled()) return;

            Material* material = mr.material;
            Mesh* mesh = mr.mesh;
            const Buffer* candidates = gpu_instances.forward_candidates();
            const std::uint32_t candidate_count = gpu_instances.forward_count();

            if (!material || !mesh || !candidates || candidate_count == 0u) return;

            MaterialRenderInfo* render_info = get_or_build_render_info(material);
            if (!render_info || !render_info->instancing_range_index.has_value()) return;

            const std::size_t range_index = *render_info->instancing_range_index;
            if (range_index >= render_info->ranges.size()) return;

            const PushConstantRangeRuntime& range = render_info->ranges[range_index];
            if (!supports_gpu_culled_instancing(range)) return;

            GpuMesh* gpu_mesh = get_or_create_gpu_mesh(mesh);
            if (!gpu_mesh) return;

            GpuDrivenBatchState* state = get_gpu_driven_batch_state(entity.id());
            if (!state) return;

            const std::size_t required_visible_bytes =
                static_cast<std::size_t>(candidate_count) * static_cast<std::size_t>(range.instancing_stride);

            if (!ensure_gpu_driven_batch_capacity(*state, required_visible_bytes)) return;

            (void)dispatch_gpu_driven_cull(
                *candidates,
                candidate_count,
                *gpu_mesh,
                *state->params_buffer,
                *state->visible_buffer,
                *state->indirect_buffer,
                gpu_cull_frustum_planes_,
                *gpu_driven_forward_dispatcher);
        });
    }

    bool RenderingSystem::execute_clustered_light_culling_pass()
    {
        auto& material_loader = gfx::MaterialLoader::instance();

        Buffer* lighting_buffer = material_loader.lighting_ubo();
        Buffer* camera_buffer = material_loader.camera_ubo();
        Buffer* point_lights_buffer = material_loader.point_lights_buffer();
        Buffer* spot_lights_buffer = material_loader.spot_lights_buffer();
        Buffer* cluster_records_buffer = material_loader.cluster_records_buffer();
        Buffer* cluster_light_indices_buffer = material_loader.cluster_light_indices_buffer();
        Buffer* cluster_light_counter_buffer = material_loader.cluster_light_counter_buffer();

        if (!lighting_buffer ||
            !camera_buffer ||
            !point_lights_buffer ||
            !spot_lights_buffer ||
            !cluster_records_buffer ||
            !cluster_light_indices_buffer ||
            !cluster_light_counter_buffer)
        {
            return false;
        }

        ComputeDispatcher* cluster_build_dispatcher = get_frame_dispatcher(cluster_build_dispatchers_, "cluster_build");
        ComputeDispatcher* light_cull_dispatcher = get_frame_dispatcher(light_cull_dispatchers_, "light_cull");

        if (!cluster_build_dispatcher ||
            !light_cull_dispatcher ||
            cluster_build_dispatcher->failed() ||
            light_cull_dispatcher->failed())
        {
            return false;
        }

        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!cmd) return false;

        const std::uint32_t cluster_count = std::max(material_loader.cluster_record_count(), 1u);

        cluster_build_dispatcher
            ->set("lighting_data", *lighting_buffer)
            .set("cluster_records", *cluster_records_buffer)
            .set("cluster_light_indices", *cluster_light_indices_buffer)
            .set("cluster_light_counter", *cluster_light_counter_buffer)
            .dispatch_on_current_command_buffer(cluster_count);

        if (cluster_build_dispatcher->failed()) return false;

        cmd->compute_memory_barrier();

        light_cull_dispatcher
            ->set("camera_data", *camera_buffer)
            .set("lighting_data", *lighting_buffer)
            .set("point_lights", *point_lights_buffer)
            .set("spot_lights", *spot_lights_buffer)
            .set("cluster_records", *cluster_records_buffer)
            .set("cluster_light_indices", *cluster_light_indices_buffer)
            .set("cluster_light_counter", *cluster_light_counter_buffer)
            .dispatch_on_current_command_buffer(cluster_count);

        return !light_cull_dispatcher->failed();
    }

    bool RenderingSystem::execute_ssao_pass()
    {
        auto& material_loader = gfx::MaterialLoader::instance();
        Buffer* camera_buffer = material_loader.camera_ubo();
        Buffer* lighting_buffer = material_loader.lighting_ubo();
        Texture* depth_texture = material_loader.frame_depth_texture();
        Texture* ssao_texture = material_loader.ssao_texture();
        Sampler* renderer_sampler = material_loader.renderer_sampler();
        if (!camera_buffer || !lighting_buffer || !depth_texture || !ssao_texture || !renderer_sampler) return false;

        ComputeDispatcher* ssao_dispatcher = get_frame_dispatcher(ssao_dispatchers_, "ssao");
        if (!ssao_dispatcher || ssao_dispatcher->failed()) return false;

        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!cmd) return false;

        auto* depth_handle = static_cast<rhi::Texture*>(TextureAccess::handle(*depth_texture));
        auto* ssao_handle = static_cast<rhi::Texture*>(TextureAccess::handle(*ssao_texture));
        if (!depth_handle || !ssao_handle) return false;

        const TextureLayout current_ssao_layout = TextureAccess::layout(*ssao_texture);
        const rhi::ResourceState previous_ssao_state =
            current_ssao_layout == TextureLayout::ShaderReadOnly
                ? rhi::ResourceState::ShaderResource
                : rhi::ResourceState::Undefined;

        cmd->image_barrier(
            ssao_handle,
            previous_ssao_state,
            rhi::ResourceState::UnorderedAccess);
        TextureAccess::set_layout(*ssao_texture, TextureLayout::General);

        ssao_dispatcher
            ->set("camera_data", *camera_buffer)
            .set("lighting_data", *lighting_buffer)
            .set("depth_texture", *depth_texture, *renderer_sampler)
            .set("ssao_texture", *ssao_texture)
            .dispatch_on_current_command_buffer(ssao_texture->width, ssao_texture->height);

        if (ssao_dispatcher->failed()) return false;

        cmd->image_barrier(
            ssao_handle,
            rhi::ResourceState::UnorderedAccess,
            rhi::ResourceState::ShaderResource);
        TextureAccess::set_layout(*ssao_texture, TextureLayout::ShaderReadOnly);

        return true;
    }

    void RenderingSystem::execute_forward_pass()
    {
        auto* swapchain = rhi::RenderContext::swapchain();
        if (!swapchain) return;

        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!cmd) return;

        if (gpu_indirect_instancing_enabled_)
        {
            cmd->compute_to_draw_barrier();
        }

        const std::uint32_t image_index = swapchain->current_image_index();
        if (image_index == rhi::Swapchain::invalid_image_index || image_index == rhi::Swapchain::skip_image_index)
            return;

        auto& material_loader = gfx::MaterialLoader::instance();
        const bool use_swapchain_msaa = swapchain->sample_count() != rhi::TextureSampleCount::Count1;

        rhi::Texture* frame_depth_handle = nullptr;
        Texture* frame_depth_texture = material_loader.frame_depth_texture();

        if (!use_swapchain_msaa && frame_depth_texture)
        {
            frame_depth_handle = static_cast<rhi::Texture*>(TextureAccess::handle(*frame_depth_texture));

            if (frame_depth_handle)
            {
                cmd->image_barrier(
                    frame_depth_handle,
                    TextureAccess::layout(*frame_depth_texture) == TextureLayout::ShaderReadOnly
                        ? rhi::ResourceState::ShaderResource
                        : rhi::ResourceState::Undefined,
                    rhi::ResourceState::DepthStencil);
                TextureAccess::set_layout(*frame_depth_texture, TextureLayout::DepthStencilAttachment);
            }
        }

        if (!swapchain->begin_render_pass(image_index, frame_depth_handle, frame_depth_handle == nullptr))
        {
            Log::error("Failed to begin swapchain render pass for clustered_forward");
            return;
        }

        submit_draws(false);

        if (!swapchain->end_render_pass(image_index))
            Log::error("Failed to end swapchain render pass for clustered_forward");
    }

    void RenderingSystem::submit_draws(const bool bind_frame_resources)
    {
        assert_render_thread();

        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!cmd) return;

        static std::uint32_t submit_log_frame = 0;
        const bool log_submit_frame = submit_log_frame < 4 || submit_log_frame % 5000 == 0;
        if (log_submit_frame)
        {
            Log::debug(
                "submit_draws frame {}: pipelines={} gpu_indirect_instancing_enabled={}",
                submit_log_frame,
                pipeline_materials_.size(),
                gpu_indirect_instancing_enabled_);
        }

        constexpr std::size_t max_instance_index = std::numeric_limits<std::uint32_t>::max();

        for (auto& [pipeline_ptr, materials] : pipeline_materials_)
        {
            cmd->bind_graphics_pipeline(pipeline_ptr);

            for (Material* material : materials)
            {
                if (material->shadow_only()) continue;

                auto mat_it = render_cache_.find(material);
                if (mat_it == render_cache_.end()) continue;

                const MaterialRenderGroup& mat_group = mat_it->second;
                if (mat_group.num_visible == 0) continue;

                if (log_submit_frame)
                {
                    Log::debug(
                        "submit_draws material='{}' visible={} mesh_buckets={}",
                        material->name(),
                        mat_group.num_visible,
                        mat_group.mesh_buckets.size());
                }

                MaterialRenderInfo* render_info = get_or_build_render_info(material);

                if (bind_frame_resources)
                {
                    ensure_fallback_ssbo_bound(*material, render_info);
                    gfx::MaterialLoader::instance().bind_engine_resources(material);
                    bind_frame_render_resources(*material);
                }
                MaterialAccess::bind_descriptor_sets(*material);

                draw_material_meshes(cmd, material, mat_group, render_info, max_instance_index);
            }
        }

        Scene::world().each(
            [&](const flecs::entity entity, MeshRenderer& mr, GpuDrivenInstances& gpu_instances)
        {
            if (!entity.is_valid() || !entity.enabled()) return;
            if (entity.has<tags::ShadowOnly>()) return;

            Material* material = mr.material;
            Mesh* mesh = mr.mesh;
            const std::uint32_t candidate_count = gpu_instances.forward_count();

            if (!material || !mesh || candidate_count == 0u || material->shadow_only()) return;

            MaterialRenderInfo* render_info = get_or_build_render_info(material);
            if (!render_info || !render_info->instancing_range_index.has_value()) return;

            const std::size_t range_index = *render_info->instancing_range_index;
            if (range_index >= render_info->ranges.size()) return;

            const PushConstantRangeRuntime& range = render_info->ranges[range_index];
            if (!supports_gpu_culled_instancing(range)) return;

            GpuMesh* gpu_mesh = get_or_create_gpu_mesh(mesh);
            if (!gpu_mesh) return;

            GpuDrivenBatchState* state = get_gpu_driven_batch_state(entity.id());
            if (!state || !state->visible_buffer || !state->indirect_buffer) return;

            auto* pipeline = static_cast<rhi::GraphicsPipeline*>(MaterialAccess::pipeline(*material));
            auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->vertex_buffer));
            auto* index_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->index_buffer));
            auto* indirect_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(*state->indirect_buffer));

            if (!pipeline || !vertex_buffer_rhi || !index_buffer_rhi || !indirect_buffer_rhi) return;

            cmd->bind_graphics_pipeline(pipeline);
            cmd->bind_vertex_buffer(vertex_buffer_rhi);
            cmd->bind_index_buffer(index_buffer_rhi);

            material->update_buffer(range.instancing_ssbo_name, *state->visible_buffer);
            MaterialAccess::bind_descriptor_sets(*material);
            push_ranges(cmd, *material, render_info, nullptr, true, nullptr, false);
            cmd->draw_indexed_indirect(
                indirect_buffer_rhi,
                0,
                1,
                sizeof(GpuIndexedDrawCommand));
        });

        ++submit_log_frame;
    }

    void RenderingSystem::draw_material_meshes(
        rhi::CommandBuffer* cmd,
        Material* material,
        const MaterialRenderGroup& mat_group,
        void* render_info_opaque,
        const std::size_t max_instance_index)
    {
        auto* render_info = static_cast<MaterialRenderInfo*>(render_info_opaque);

        const PushConstantRangeRuntime* direct_instancing_range = nullptr;
        bool supports_direct_instancing = false;
        if (render_info && render_info->instancing_range_index.has_value())
        {
            const std::size_t range_index = *render_info->instancing_range_index;
            if (range_index < render_info->ranges.size())
            {
                const PushConstantRangeRuntime& range = render_info->ranges[range_index];
                supports_direct_instancing =
                    !gpu_indirect_instancing_enabled_ &&
                    range.has_model_field &&
                    range.model_offset_in_range >= range.data_offset_in_range &&
                    range.model_offset_in_range == range.data_offset_in_range &&
                    range.data_size == sizeof(glm::mat4) &&
                    range.instancing_stride == sizeof(glm::mat4);
                if (supports_direct_instancing)
                    direct_instancing_range = &range;
            }
        }

        struct InstancedDrawPlan final
        {
            Mesh* mesh_ptr{ nullptr };
            GpuMesh* gpu_mesh{ nullptr };
            std::uint32_t first_instance{ 0 };
            std::uint32_t instance_count{ 0 };
        };

        std::vector<InstancedDrawPlan> instanced_plans{};
        std::vector<glm::mat4> instanced_models{};
        flat_set<Mesh*> instanced_meshes{};

        if (supports_direct_instancing && direct_instancing_range)
        {
            for (auto& [mesh_ptr, bucket] : mat_group.mesh_buckets)
            {
                if (bucket.num_visible < instancing_threshold_ || bucket.num_visible > max_instance_index) continue;

                auto* gpu_mesh = get_or_create_gpu_mesh(mesh_ptr);
                if (!gpu_mesh) continue;

                const std::uint32_t first_instance = static_cast<std::uint32_t>(instanced_models.size());

                for (const auto& elem : bucket.elements)
                {
                    if (!elem.visible) continue;

                    const auto* transform = std::as_const(elem.entity).try_get_component<Transform>();
                    if (!transform) continue;

                    instanced_models.push_back(transform->world_matrix());
                }

                const std::uint32_t instance_count = static_cast<std::uint32_t>(instanced_models.size()) - first_instance;
                if (instance_count == 0) continue;

                instanced_plans.push_back(InstancedDrawPlan{
                    .mesh_ptr = mesh_ptr,
                    .gpu_mesh = gpu_mesh,
                    .first_instance = first_instance,
                    .instance_count = instance_count
                });
                instanced_meshes.insert(mesh_ptr);
            }

            if (!instanced_plans.empty() &&
                upload_instance_payload_from_models(material, instanced_models, *direct_instancing_range))
            {
                MaterialAccess::bind_descriptor_sets(*material);

                for (const InstancedDrawPlan& plan : instanced_plans)
                {
                    auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(plan.gpu_mesh->vertex_buffer));
                    auto* index_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(plan.gpu_mesh->index_buffer));
                    if (!vertex_buffer_rhi || !index_buffer_rhi) continue;

                    cmd->bind_vertex_buffer(vertex_buffer_rhi);
                    cmd->bind_index_buffer(index_buffer_rhi);
                    push_ranges(cmd, *material, render_info, nullptr, true, nullptr, false);
                    cmd->draw_indexed(
                        plan.gpu_mesh->index_count,
                        plan.instance_count,
                        0,
                        0,
                        plan.first_instance);
                }
            }
            else
            {
                instanced_meshes.clear();
            }
        }

        for (auto& [mesh_ptr, bucket] : mat_group.mesh_buckets)
        {
            if (bucket.num_visible == 0) continue;
            if (instanced_meshes.contains(mesh_ptr)) continue;

            auto* gpu_mesh = get_or_create_gpu_mesh(mesh_ptr);
            if (!gpu_mesh) continue;

            auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->vertex_buffer));
            auto* index_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->index_buffer));
            if (!vertex_buffer_rhi || !index_buffer_rhi) continue;

            cmd->bind_vertex_buffer(vertex_buffer_rhi);
            cmd->bind_index_buffer(index_buffer_rhi);

            const bool should_instance =
                render_info &&
                render_info->instancing_range_index.has_value() &&
                bucket.num_visible >= instancing_threshold_ &&
                bucket.num_visible <= max_instance_index;

            static std::uint32_t draw_log_count = 0;
            if (draw_log_count < 24 && (should_instance || bucket.elements.size() >= instancing_threshold_))
            {
                Log::debug(
                    "draw bucket: material='{}' mesh='{}' total={} visible={} should_instance={}",
                    material->name(),
                    mesh_ptr->name(),
                    bucket.elements.size(),
                    bucket.num_visible,
                    should_instance);
                ++draw_log_count;
            }

            if (should_instance && try_instanced_draw(cmd, material, mesh_ptr, bucket, render_info, gpu_mesh))
                continue;

            draw_elements_individually(cmd, material, bucket, render_info, gpu_mesh);
        }
    }

    bool RenderingSystem::try_instanced_draw(
        rhi::CommandBuffer* cmd,
        Material* material,
        Mesh* mesh,
        const MeshBucket& bucket,
        void* render_info_opaque,
        GpuMesh* gpu_mesh)
    {
        (void)bucket;
        (void)gpu_mesh;

        auto* render_info = static_cast<MaterialRenderInfo*>(render_info_opaque);
        const std::size_t range_index = *render_info->instancing_range_index;
        if (range_index >= render_info->ranges.size())
            return false;

        const PushConstantRangeRuntime& range = render_info->ranges[range_index];

        if (gpu_indirect_instancing_enabled_ &&
            frustum_.valid &&
            supports_gpu_culled_instancing(range))
        {
            GpuCullBufferState* state = get_gpu_cull_buffer_state(material, mesh);
            if (!state || !state->culled_instance_buffer || !state->indirect_buffer) return false;

            auto* indirect_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(*state->indirect_buffer));
            if (!indirect_buffer_rhi) return false;

            const auto visible_handle = BufferAccess::handle(*state->culled_instance_buffer);
            if (state->bound_culled_handle != visible_handle)
            {
                material->update_buffer(range.instancing_ssbo_name, *state->culled_instance_buffer);
                state->bound_culled_handle = visible_handle;
            }

            push_ranges(cmd, *material, render_info, nullptr, true, nullptr, false);
            cmd->draw_indexed_indirect(
                indirect_buffer_rhi,
                0,
                1,
                sizeof(GpuIndexedDrawCommand));

            return true;
        }

        return false;

    }

    void RenderingSystem::draw_elements_individually(
        rhi::CommandBuffer* cmd,
        Material* material,
        const MeshBucket& bucket,
        void* render_info_opaque,
        GpuMesh* gpu_mesh)
    {
        auto* render_info = static_cast<MaterialRenderInfo*>(render_info_opaque);
        for (const auto& elem : bucket.elements)
        {
            if (!elem.visible) continue;

            const auto* transform = std::as_const(elem.entity).try_get_component<Transform>();
            if (!transform) continue;

            const glm::mat4 model_matrix = transform->world_matrix();

            push_ranges(cmd, *material, render_info, &model_matrix, false, nullptr, false);
            cmd->draw_indexed(gpu_mesh->index_count);
        }
    }

    void RenderingSystem::EndFrame::execute()
    {
        assert_render_thread();

        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!swapchain_ || !cmd)
        {
            frame_active_ = false;
            return;
        }

        if (frame_active_)
        {
            auto& material_loader = gfx::MaterialLoader::instance();

            material_loader.ensure_frame_render_targets(swapchain_->width(), swapchain_->height());
            prepare_frame_material_bindings();

            execute_shadow_caster_cull_pass();
            execute_directional_shadow_pass();
            execute_point_shadow_pass();
            execute_spot_shadow_pass();
            execute_depth_prepass();

            static bool clustered_light_culling_warned = false;
            if (!execute_clustered_light_culling_pass() && !clustered_light_culling_warned)
            {
                Log::warn("Clustered light culling pass failed; rendering will use the last uploaded cluster data");
                clustered_light_culling_warned = true;
            }

            execute_gpu_instance_culling_pass();
            execute_forward_pass();
        }

        rhi::RenderContext::set_current_command_buffer(nullptr);
        const rhi::PresentResult end_result = swapchain_->end_frame_result();
        if (end_result == rhi::PresentResult::Error) Log::error("Swapchain end_frame failed");

        frame_active_ = false;
    }
}
