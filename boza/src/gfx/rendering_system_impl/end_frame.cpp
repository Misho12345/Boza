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
import :material_loader;

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

    namespace
    {
        struct StaticMeshBatch final
        {
            Material* material{ nullptr };
            Mesh* mesh{ nullptr };
            std::vector<glm::mat4> models{};
        };

        StaticMeshBatch& get_or_add_batch(
            std::vector<StaticMeshBatch>& batches,
            Material* material,
            Mesh* mesh)
        {
            if (auto it = std::ranges::find_if(
                batches,
                [material, mesh](const StaticMeshBatch& batch)
                {
                    return batch.material == material && batch.mesh == mesh;
                }); it != batches.end())
            {
                return *it;
            }

            batches.push_back(StaticMeshBatch{
                .material = material,
                .mesh = mesh
            });

            return batches.back();
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

            std::vector<StaticMeshBatch> standard_shadow_batches{};
            flat_map<Material*, std::vector<StaticMeshBatch*>> shadow_batches_by_material{};

            Scene::world().each(
                [&](const flecs::entity entity, const Transform& transform, const ShadowCaster& caster, MeshRenderer& mr)
            {
                if (!entity.is_valid() || !entity.enabled()) return;
                if (entity.has<GpuDrivenInstances>()) return;

                Material* material = mr.material;
                Mesh* mesh = mr.mesh;

                if (!material || !mesh) return;
                if (!ensure_shadow_pipeline(material)) return;

                ShadowCasterGeometry geometry{};
                if (!build_shadow_caster_geometry(transform, caster, mesh, geometry)) return;
                if (!shadow_frustum.sphere_visible(geometry.center, geometry.sphere.w)) return;

                get_or_add_batch(standard_shadow_batches, material, mesh)
                    .models.push_back(transform.world_matrix());
            });

            for (auto& batch : standard_shadow_batches)
                shadow_batches_by_material[batch.material].push_back(&batch);

            for (auto& [material, batch_list] : shadow_batches_by_material)
            {
                if (!material || batch_list.empty()) continue;

                MaterialRenderInfo* render_info = get_or_build_render_info(material);
                auto shadow_pipeline_it = shadow_pipelines_.find(material);
                if (shadow_pipeline_it == shadow_pipelines_.end()) continue;

                const PushConstantRangeRuntime* instancing_range = nullptr;
                if (render_info && render_info->instancing_range_index.has_value())
                {
                    const std::size_t range_index = *render_info->instancing_range_index;
                    if (range_index < render_info->ranges.size())
                    {
                        const PushConstantRangeRuntime& candidate = render_info->ranges[range_index];
                        if (candidate.instancing_supported &&
                            candidate.has_model_field &&
                            candidate.model_offset_in_range >= candidate.data_offset_in_range &&
                            candidate.model_offset_in_range + sizeof(glm::mat4) <=
                                candidate.data_offset_in_range + candidate.data_size)
                        {
                            instancing_range = &candidate;
                        }
                    }
                }

                struct InstancedDrawPlan final
                {
                    GpuMesh* gpu_mesh{ nullptr };
                    std::uint32_t first_instance{ 0 };
                    std::uint32_t instance_count{ 0 };
                    Mesh* mesh{ nullptr };
                };

                std::vector<InstancedDrawPlan> instanced_plans{};
                std::vector<glm::mat4> instanced_models{};
                flat_set<Mesh*> instanced_meshes{};

                if (instancing_range)
                {
                    for (StaticMeshBatch* batch : batch_list)
                    {
                        if (!batch || !batch->mesh || batch->models.size() < instancing_threshold_) continue;
                        if (batch->models.size() > std::numeric_limits<std::uint32_t>::max()) continue;

                        GpuMesh* gpu_mesh = get_or_create_gpu_mesh(batch->mesh);
                        if (!gpu_mesh) continue;

                        const std::uint32_t first_instance = static_cast<std::uint32_t>(instanced_models.size());
                        instanced_models.insert(instanced_models.end(), batch->models.begin(), batch->models.end());

                        instanced_plans.push_back(InstancedDrawPlan{
                            .gpu_mesh = gpu_mesh,
                            .first_instance = first_instance,
                            .instance_count = static_cast<std::uint32_t>(batch->models.size()),
                            .mesh = batch->mesh
                        });
                        instanced_meshes.insert(batch->mesh);
                    }
                }

                cmd->bind_graphics_pipeline(shadow_pipeline_it->second.pipeline.get());

                bool drew_instanced_batches = false;
                if (!instanced_plans.empty() &&
                    upload_instance_payload_from_models(material, std::span<const glm::mat4>{ instanced_models }, *instancing_range))
                {
                    MaterialAccess::bind_descriptor_sets(*material);

                    for (const InstancedDrawPlan& plan : instanced_plans)
                    {
                        auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(plan.gpu_mesh->vertex_buffer));
                        auto* index_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(plan.gpu_mesh->index_buffer));
                        if (!vertex_buffer_rhi || !index_buffer_rhi) continue;

                        cmd->bind_vertex_buffer(vertex_buffer_rhi);
                        cmd->bind_index_buffer(index_buffer_rhi);
                        push_ranges(cmd, *material, render_info, nullptr, true, &light_view_projection, true);
                        cmd->draw_indexed(plan.gpu_mesh->index_count, plan.instance_count, 0, 0, plan.first_instance);
                        shadow_draws += plan.instance_count;
                    }

                    drew_instanced_batches = true;
                }

                MaterialAccess::bind_descriptor_sets(*material);

                for (StaticMeshBatch* batch : batch_list)
                {
                    if (!batch || !batch->mesh || batch->models.empty()) continue;
                    if (drew_instanced_batches && instanced_meshes.contains(batch->mesh)) continue;

                    GpuMesh* gpu_mesh = get_or_create_gpu_mesh(batch->mesh);
                    if (!gpu_mesh) continue;

                    auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->vertex_buffer));
                    auto* index_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->index_buffer));
                    if (!vertex_buffer_rhi || !index_buffer_rhi) continue;

                    cmd->bind_vertex_buffer(vertex_buffer_rhi);
                    cmd->bind_index_buffer(index_buffer_rhi);

                    for (const glm::mat4& model_matrix : batch->models)
                    {
                        push_ranges(cmd, *material, render_info, &model_matrix, false, &light_view_projection, true);
                        cmd->draw_indexed(gpu_mesh->index_count);
                        ++shadow_draws;
                    }
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
        auto& material_loader = gfx::MaterialLoader::instance();

        Scene::world().each(
            [&](const flecs::entity entity, MeshRenderer& mr)
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

        std::vector<StaticMeshBatch> standard_forward_batches{};
        flat_map<Material*, std::vector<StaticMeshBatch*>> forward_batches_by_material{};

        Scene::world().each(
            [&](const flecs::entity entity, const Transform& transform, MeshRenderer& mr)
        {
            if (!entity.is_valid() || !entity.enabled()) return;
            if (entity.has<tags::ShadowOnly>() || entity.has<GpuDrivenInstances>()) return;

            Material* material = mr.material;
            Mesh* mesh = mr.mesh;

            if (!material || !mesh || material->shadow_only()) return;

            const glm::mat4 model_matrix = transform.world_matrix();

            if (frustum_.valid)
            {
                const BoundingSphere& mesh_bounds = mesh->bounds;
                const glm::vec3 world_center =
                    glm::vec3(model_matrix * glm::vec4{ mesh_bounds.center, 1.0f });

                const float scale_x = glm::length(glm::vec3{ model_matrix[0] });
                const float scale_y = glm::length(glm::vec3{ model_matrix[1] });
                const float scale_z = glm::length(glm::vec3{ model_matrix[2] });

                const float world_radius = mesh_bounds.radius * std::max({ scale_x, scale_y, scale_z });
                if (!frustum_.sphere_visible(world_center, world_radius)) return;
            }

            get_or_add_batch(standard_forward_batches, material, mesh)
                .models.push_back(model_matrix);
        });

        for (auto& batch : standard_forward_batches)
            forward_batches_by_material[batch.material].push_back(&batch);

        for (auto& [material, batch_list] : forward_batches_by_material)
        {
            if (!material || batch_list.empty()) continue;

            MaterialRenderInfo* render_info = get_or_build_render_info(material);

            const PushConstantRangeRuntime* instancing_range = nullptr;
            if (render_info && render_info->instancing_range_index.has_value())
            {
                const std::size_t range_index = *render_info->instancing_range_index;
                if (range_index < render_info->ranges.size())
                {
                    const PushConstantRangeRuntime& candidate = render_info->ranges[range_index];
                    if (candidate.instancing_supported &&
                        candidate.has_model_field &&
                        candidate.model_offset_in_range >= candidate.data_offset_in_range &&
                        candidate.model_offset_in_range + sizeof(glm::mat4) <=
                            candidate.data_offset_in_range + candidate.data_size)
                    {
                        instancing_range = &candidate;
                    }
                }
            }

            struct InstancedDrawPlan final
            {
                GpuMesh* gpu_mesh{ nullptr };
                std::uint32_t first_instance{ 0 };
                std::uint32_t instance_count{ 0 };
                Mesh* mesh{ nullptr };
            };

            std::vector<InstancedDrawPlan> instanced_plans{};
            std::vector<glm::mat4> instanced_models{};
            flat_set<Mesh*> instanced_meshes{};

            if (instancing_range)
            {
                for (StaticMeshBatch* batch : batch_list)
                {
                    if (!batch || !batch->mesh || batch->models.size() < instancing_threshold_) continue;
                    if (batch->models.size() > std::numeric_limits<std::uint32_t>::max()) continue;

                    GpuMesh* gpu_mesh = get_or_create_gpu_mesh(batch->mesh);
                    if (!gpu_mesh) continue;

                    const std::uint32_t first_instance = static_cast<std::uint32_t>(instanced_models.size());
                    instanced_models.insert(instanced_models.end(), batch->models.begin(), batch->models.end());

                    instanced_plans.push_back(InstancedDrawPlan{
                        .gpu_mesh = gpu_mesh,
                        .first_instance = first_instance,
                        .instance_count = static_cast<std::uint32_t>(batch->models.size()),
                        .mesh = batch->mesh
                    });
                    instanced_meshes.insert(batch->mesh);
                }
            }

            auto* pipeline = static_cast<rhi::GraphicsPipeline*>(MaterialAccess::pipeline(*material));
            if (!pipeline) continue;

            cmd->bind_graphics_pipeline(pipeline);

            bool drew_instanced_batches = false;
            if (!instanced_plans.empty() &&
                upload_instance_payload_from_models(material, std::span<const glm::mat4>{ instanced_models }, *instancing_range))
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
                    cmd->draw_indexed(plan.gpu_mesh->index_count, plan.instance_count, 0, 0, plan.first_instance);
                }

                drew_instanced_batches = true;
            }

            MaterialAccess::bind_descriptor_sets(*material);

            for (StaticMeshBatch* batch : batch_list)
            {
                if (!batch || !batch->mesh || batch->models.empty()) continue;
                if (drew_instanced_batches && instanced_meshes.contains(batch->mesh)) continue;

                GpuMesh* gpu_mesh = get_or_create_gpu_mesh(batch->mesh);
                if (!gpu_mesh) continue;

                auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->vertex_buffer));
                auto* index_buffer_rhi = static_cast<rhi::Buffer*>(BufferAccess::handle(gpu_mesh->index_buffer));
                if (!vertex_buffer_rhi || !index_buffer_rhi) continue;

                cmd->bind_vertex_buffer(vertex_buffer_rhi);
                cmd->bind_index_buffer(index_buffer_rhi);

                for (const glm::mat4& model_matrix : batch->models)
                {
                    push_ranges(cmd, *material, render_info, &model_matrix, false, nullptr, false);
                    cmd->draw_indexed(gpu_mesh->index_count);
                }
            }
        }

        submit_gpu_driven_draws();

        if (!swapchain->end_render_pass(image_index))
            Log::error("Failed to end swapchain render pass for clustered_forward");
    }

    void RenderingSystem::submit_gpu_driven_draws()
    {
        assert_render_thread();

        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!cmd) return;

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
