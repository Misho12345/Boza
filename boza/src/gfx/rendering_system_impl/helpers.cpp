module boza.gfx;

import :buffer;
import :material;
import :rendering_system;
import :rendering_system_common;

import boza.core;
import boza.rhi;
import boza.rhi;
import :material_loader;

namespace boza
{
    namespace
    {
        std::mutex       render_thread_mutex_{};
        std::thread::id  render_thread_{};
        std::atomic_bool render_thread_set_{ false };

        [[nodiscard]] glm::vec3 normalized_axis_or_fallback(
            const glm::vec3& axis,
            const glm::vec3& fallback)
        {
            if (glm::length2(axis) <= 1e-8f) return fallback;
            return glm::normalize(axis);
        }

        void build_mesh_local_bounds(
            const Mesh& mesh,
            glm::vec3& center,
            glm::vec3& half_extents)
        {
            const auto& vertices = mesh.vertices();
            if (vertices.empty())
            {
                center = glm::vec3{ 0.0f };
                half_extents = glm::vec3{ 0.0f };
                return;
            }

            glm::vec3 min_position = vertices.front().position;
            glm::vec3 max_position = vertices.front().position;

            for (const Vertex& vertex : vertices)
            {
                min_position = glm::min(min_position, vertex.position);
                max_position = glm::max(max_position, vertex.position);
            }

            center = (min_position + max_position) * 0.5f;
            half_extents = glm::max((max_position - min_position) * 0.5f, glm::vec3{ 0.0f });
        }

    }

    bool build_shadow_caster_geometry(
        const Transform& transform,
        const ShadowCaster& caster,
        const Mesh* mesh,
        ShadowCasterGeometry& geometry)
    {
        glm::vec3 local_center{ 0.0f };
        glm::vec3 local_half_extents{ 0.0f };

        if (mesh && Mesh::exists(mesh))
            build_mesh_local_bounds(*mesh, local_center, local_half_extents);

        const glm::mat4 model_matrix = transform.world_matrix();
        const glm::vec3 basis_x = glm::vec3{ model_matrix[0] };
        const glm::vec3 basis_y = glm::vec3{ model_matrix[1] };
        const glm::vec3 basis_z = glm::vec3{ model_matrix[2] };

        const glm::vec3 axis_lengths{
            glm::length(basis_x),
            glm::length(basis_y),
            glm::length(basis_z)
        };

        const float extent_scale = std::max(caster.extent_scale, 0.0f);
        const float min_extent = std::max(caster.min_extent, 0.001f);

        geometry.center = glm::vec3{ model_matrix * glm::vec4{ local_center, 1.0f } };
        geometry.axis_x = normalized_axis_or_fallback(basis_x, glm::vec3{ 1.0f, 0.0f, 0.0f });
        geometry.axis_y = normalized_axis_or_fallback(basis_y, glm::vec3{ 0.0f, 1.0f, 0.0f });
        geometry.axis_z = normalized_axis_or_fallback(basis_z, glm::vec3{ 0.0f, 0.0f, 1.0f });
        geometry.half_extents = glm::max(
            local_half_extents * axis_lengths * extent_scale,
            glm::vec3{ min_extent });
        geometry.sphere = glm::vec4{
            geometry.center,
            std::max(glm::length(geometry.half_extents), min_extent)
        };

        return true;
    }

    void assert_render_thread()
    {
        #ifdef BOZA_DEBUG
        const std::thread::id current_thread = std::this_thread::get_id();

        if (!render_thread_set_.load(std::memory_order_acquire))
        {
            std::scoped_lock lock{ render_thread_mutex_ };

            if (!render_thread_set_.load(std::memory_order_relaxed))
            {
                render_thread_ = current_thread;
                render_thread_set_.store(true, std::memory_order_release);
                return;
            }
        }

        assert(
            render_thread_ == current_thread,
            "RenderingSystem render cache accessed from a non-owner thread"
        );
        #endif
    }

    void clear_render_thread()
    {
        #ifdef BOZA_DEBUG
        std::scoped_lock lock{ render_thread_mutex_ };
        render_thread_ = {};
        render_thread_set_.store(false, std::memory_order_release);
        #endif
    }

    std::uint32_t active_frame_index()
    {
        const auto* swapchain = rhi::RenderContext::swapchain();
        return swapchain ? swapchain->current_frame() : 0u;
    }

    std::uint32_t active_frame_slot_count()
    {
        return std::max(rhi::RenderContext::frames_in_flight(), 1u);
    }

    void retire_buffer(std::unique_ptr<Buffer>&& buffer)
    {
        if (!buffer) return;

        retired_buffers_.push_back(RetiredBuffer{
            .release_frame = Time::frame_count() + std::max<std::uint64_t>(active_frame_slot_count(), 1u) + 1u,
            .buffer = std::move(buffer)
        });
    }

    void collect_retired_buffers()
    {
        const std::uint64_t current_frame = Time::frame_count();

        std::erase_if(
            retired_buffers_,
            [current_frame](const RetiredBuffer& retired)
            {
                return retired.release_frame <= current_frame;
            });
    }

    ComputeDispatcher* get_frame_dispatcher(
        std::vector<std::unique_ptr<ComputeDispatcher>>& dispatchers,
        const std::string_view shader_name)
    {
        const std::uint32_t frame_slot_count = active_frame_slot_count();
        if (dispatchers.size() < frame_slot_count)
            dispatchers.resize(frame_slot_count);

        const std::uint32_t frame_index = active_frame_index() % frame_slot_count;
        if (!dispatchers[frame_index])
            dispatchers[frame_index] = std::make_unique<ComputeDispatcher>(std::string{ shader_name });

        return dispatchers[frame_index].get();
    }

    [[nodiscard]]
    static bool is_within_range(
        const std::uint32_t member_offset,
        const std::uint32_t member_size,
        const std::uint32_t range_offset,
        const std::uint32_t range_size)
    {
        if (member_offset < range_offset) return false;

        const std::uint64_t member_end =
            static_cast<std::uint64_t>(member_offset) + static_cast<std::uint64_t>(member_size);
        const std::uint64_t range_end =
            static_cast<std::uint64_t>(range_offset) + static_cast<std::uint64_t>(range_size);

        return member_end <= range_end;
    }

    RenderingSystem::FrustumPlane RenderingSystem::FrustumState::normalize_plane(const glm::vec4& plane)
    {
        const glm::vec3 normal{ plane.x, plane.y, plane.z };
        const float normal_length = glm::length(normal);

        if (normal_length <= std::numeric_limits<float>::epsilon()) return {};

        return {
            .normal   = normal / normal_length,
            .distance = plane.w / normal_length
        };
    }

    void RenderingSystem::FrustumState::set_from_view_projection(const glm::mat4& vp)
    {
        const glm::mat4 m = transpose(vp);

        planes[0] = normalize_plane(m[3] + m[0]);
        planes[1] = normalize_plane(m[3] - m[0]);
        planes[2] = normalize_plane(m[3] + m[1]);
        planes[3] = normalize_plane(m[3] - m[1]);
        // Boza uses a Vulkan/D3D-style 0..1 depth range, so the near plane is row2.
        planes[4] = normalize_plane(m[2]);
        planes[5] = normalize_plane(m[3] - m[2]);

        valid = true;
    }

    bool RenderingSystem::FrustumState::sphere_visible(const glm::vec3& center, const float radius) const
    {
        if (!valid) return true;

        for (const FrustumPlane& plane : planes)
        {
            const float signed_distance = dot(plane.normal, center) + plane.distance;
            if (signed_distance < -radius) return false;
        }

        return true;
    }

    bool RenderingSystem::check_mesh_valid(Mesh* mesh)
    {
        if (!mesh) return false;
        if (valid_meshes_.contains(mesh)) return true;
        if (invalid_meshes_.contains(mesh)) return false;

        if (Mesh::exists(mesh))
        {
            valid_meshes_.insert(mesh);
            return true;
        }

        invalid_meshes_.insert(mesh);
        return false;
    }

    bool RenderingSystem::check_material_valid(Material* material)
    {
        if (!material) return false;
        if (valid_materials_.contains(material)) return true;
        if (invalid_materials_.contains(material)) return false;

        if (Material::exists(material))
        {
            valid_materials_.insert(material);
            return true;
        }

        invalid_materials_.insert(material);
        return false;
    }

    void reset_render_caches()
    {
        for (auto& per_binding : instance_buffers_ | std::views::values)
        {
            for (auto& state : per_binding | std::views::values)
            {
                if (!state.buffer) continue;

                state.bound_handle = nullptr;
            }
        }

        material_render_infos_.clear();
        instance_buffers_.clear();
        gpu_driven_batch_states_.clear();
        retired_buffers_.clear();
        shadow_pipelines_.clear();
        shadow_camera_buffer_.reset();
        cluster_build_dispatchers_.clear();
        light_cull_dispatchers_.clear();
        ssao_dispatchers_.clear();
        gpu_driven_forward_cull_dispatchers_.clear();
        gpu_driven_shadow_cull_dispatchers_.clear();
        directional_shadow_map_layout_initialized_ = false;
        point_shadow_map_layout_initialized_ = false;
        spot_shadow_map_layout_initialized_ = false;
        push_constant_scratch_.clear();
        instance_payload_scratch_.clear();
        gpu_cull_instance_scratch_.clear();
        shadow_instance_models_.clear();
        clear_shadow_cull_results();
    }

    void clear_shadow_cull_results()
    {
        directional_shadow_draws_.clear();
        point_shadow_draws_.clear();
        spot_shadow_draws_.clear();
    }

    bool ensure_shadow_camera_buffer()
    {
        if (shadow_camera_buffer_) return true;

        shadow_camera_buffer_ = std::make_unique<Buffer>(
            sizeof(gfx::CameraUBO),
            BufferUsage::Uniform,
            ResourceAccessMode::Dynamic);

        if (!shadow_camera_buffer_ || !BufferAccess::handle(*shadow_camera_buffer_))
        {
            Log::error("Failed to allocate shadow camera buffer");
            shadow_camera_buffer_.reset();
            return false;
        }

        return true;
    }

    bool RenderingSystem::ensure_shadow_pipeline(Material* material)
    {
        assert_render_thread();

        if (!material || !device_ || !resource_cache_) return false;

        if (auto it = shadow_pipelines_.find(material); it != shadow_pipelines_.end())
            return it->second.pipeline != nullptr && it->second.pipeline_layout != nullptr;

        const MaterialSettings& settings = MaterialAccess::settings(*material);
        if (settings.vertex_shader.empty())
        {
            Log::error("Cannot build shadow pipeline for material '{}' without a vertex shader", material->name());
            return false;
        }

        const rhi::GraphicsApi api = rhi::RenderContext::api();
        const rhi::ShaderModuleDesc vertex_desc{
            .device = device_.get(),
            .filename = settings.vertex_shader + ".vert",
            .stage = rhi::ShaderStage::Vertex
        };

        auto vertex_shader = resource_cache_->get_or_create_shader(
            vertex_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return create_shader_module(api, desc); });

        if (!vertex_shader)
        {
            Log::error("Failed to load shadow vertex shader '{}'", settings.vertex_shader);
            return false;
        }

        std::vector<rhi::DescriptorSetLayout*> set_layouts;
        set_layouts.reserve(MaterialAccess::descriptor_set_layouts(*material).size());
        for (void* layout_handle : MaterialAccess::descriptor_set_layouts(*material))
            set_layouts.push_back(static_cast<rhi::DescriptorSetLayout*>(layout_handle));

        std::vector<rhi::ShaderModule*> shaders{ vertex_shader.get() };

        auto pipeline_layout = create_pipeline_layout(
            api,
            {
                .device = device_.get(),
                .shaders = shaders,
                .set_layouts = set_layouts
            });

        if (!pipeline_layout)
        {
            Log::error("Failed to create shadow pipeline layout for material '{}'", material->name());
            return false;
        }

        rhi::PipelineBuilder builder(api, device_.get(), shaders);

        const rhi::RasterizationState rasterization{
            .cull_mode = settings.cull_mode,
            .front_face = settings.front_face,
            .depth_bias_enable = true,
            .depth_bias_constant = 0.035f,
            .depth_bias_slope = 0.45f
        };

        const rhi::DepthStencilState depth_stencil{
            .depth_test_enable = true,
            .depth_write_enable = true,
            .depth_compare_op = CompareOp::LessOrEqual
        };

        auto pipeline = builder.build_graphics_pipeline(
            pipeline_layout.get(),
            std::vector<TextureFormat>{},
            rhi::DepthFormat::D32F,
            rasterization,
            depth_stencil,
            {},
            {},
            rhi::PrimitiveTopology::TriangleList);

        if (!pipeline)
        {
            Log::error("Failed to create shadow pipeline for material '{}'", material->name());
            return false;
        }

        shadow_pipelines_.emplace(
            material,
            ShadowPipelineState{
                .vertex_shader = std::move(vertex_shader),
                .pipeline_layout = std::move(pipeline_layout),
                .pipeline = std::move(pipeline)
            });

        return true;
    }

    InstanceBufferState* get_instance_buffer(
        Material* material,
        const std::string& name)
    {
        if (!material) return nullptr;

        auto& per_binding = instance_buffers_[material];
        if (!per_binding.contains(name)) per_binding.emplace(name, InstanceBufferState{});

        auto it = per_binding.find(name);
        if (it == per_binding.end()) return nullptr;
        return &it->second;
    }

    bool ensure_buffer_capacity(
        InstanceBufferState& state,
        const std::size_t required)
    {
        if (required == 0) return true;
        if (state.buffer && state.capacity_bytes >= required) return true;

        state.bound_handle = nullptr;

        const std::size_t doubled =
            state.capacity_bytes > 0
                ? state.capacity_bytes * 2
                : initial_instance_buffer_bytes_;

        const std::size_t new_capacity = std::max({
            initial_instance_buffer_bytes_,
            doubled,
            required
        });

        auto new_buffer = std::make_unique<Buffer>(
            new_capacity,
            BufferUsage::Storage,
            ResourceAccessMode::Dynamic);

        if (!BufferAccess::handle(*new_buffer))
        {
            Log::error("Failed to allocate instance payload buffer");
            return false;
        }

        retire_buffer(std::move(state.buffer));
        state.buffer = std::move(new_buffer);
        state.capacity_bytes = new_capacity;
        return true;
    }

    namespace
    {
        bool ensure_gpu_cull_buffer_capacity(
            std::unique_ptr<Buffer>& buffer,
            std::size_t& capacity_bytes,
            const std::size_t required,
            const BufferUsage usage,
            const std::size_t initial_capacity)
        {
            if (required == 0) return true;
            if (buffer && capacity_bytes >= required) return true;

            const std::size_t doubled = capacity_bytes > 0 ? capacity_bytes * 2 : initial_capacity;
            const std::size_t new_capacity = std::max({
                initial_capacity,
                doubled,
                required
            });

            auto new_buffer = std::make_unique<Buffer>(
                new_capacity,
                usage,
                ResourceAccessMode::Dynamic);

            if (!BufferAccess::handle(*new_buffer))
            {
                Log::error("Failed to allocate GPU culling buffer");
                return false;
            }

            retire_buffer(std::move(buffer));
            buffer = std::move(new_buffer);
            capacity_bytes = new_capacity;
            return true;
        }

    }

    bool supports_gpu_culled_instancing(const PushConstantRangeRuntime& range) noexcept
    {
        return range.instancing_supported &&
            range.has_data_member &&
            range.has_model_field &&
            range.data_size == sizeof(glm::mat4) &&
            range.instancing_stride == sizeof(glm::mat4) &&
            range.model_offset_in_range == range.data_offset_in_range;
    }

    GpuDrivenBatchState* get_gpu_driven_batch_state(const std::uint64_t entity_id)
    {
        if (entity_id == 0) return nullptr;

        auto [it, inserted] = gpu_driven_batch_states_.try_emplace(entity_id, GpuDrivenBatchState{});
        if (!inserted && it == gpu_driven_batch_states_.end()) return nullptr;
        return &it->second;
    }

    bool ensure_gpu_driven_batch_capacity(
        GpuDrivenBatchState& state,
        const std::size_t required_visible_bytes)
    {
        if (!ensure_gpu_cull_buffer_capacity(
            state.params_buffer,
            state.params_capacity_bytes,
            sizeof(GpuCullParams),
            BufferUsage::Storage,
            sizeof(GpuCullParams))) return false;

        if (!ensure_gpu_cull_buffer_capacity(
            state.visible_buffer,
            state.visible_capacity_bytes,
            required_visible_bytes,
            BufferUsage::Storage,
            initial_instance_buffer_bytes_)) return false;

        if (!ensure_gpu_cull_buffer_capacity(
            state.indirect_buffer,
            state.indirect_capacity_bytes,
            sizeof(GpuIndexedDrawCommand),
            BufferUsage::StorageIndirect,
            sizeof(GpuIndexedDrawCommand))) return false;

        return state.params_buffer && state.visible_buffer && state.indirect_buffer;
    }

    bool dispatch_gpu_driven_cull(
        const Buffer& candidates,
        const std::uint32_t candidate_count,
        GpuMesh& gpu_mesh,
        Buffer& params_buffer,
        Buffer& visible_buffer,
        Buffer& indirect_buffer,
        const std::array<glm::vec4, 6>& frustum_planes,
        ComputeDispatcher& dispatcher)
    {
        if (candidate_count == 0u) return false;

        const GpuCullParams params{
            .frustum_planes = frustum_planes,
            .counts = glm::uvec4{
                candidate_count,
                gpu_mesh.index_count,
                1u,
                0u
            }
        };

        const GpuIndexedDrawCommand indirect_command{
            .index_count = gpu_mesh.index_count,
            .instance_count = 0u,
            .first_index = 0u,
            .vertex_offset = 0,
            .first_instance = 0u
        };

        params_buffer.upload(params);
        indirect_buffer.upload(indirect_command);

        dispatcher
            .set("cull_params", params_buffer)
            .set("cull_candidates", candidates)
            .set("culled_instances", visible_buffer)
            .set("indirect_draw", indirect_buffer)
            .dispatch_on_current_command_buffer(candidate_count);

        return !dispatcher.failed();
    }

    MaterialRenderInfo build_material_render_info(Material* material)
    {
        MaterialRenderInfo info{};
        if (!material) return info;

        const auto* reflection = static_cast<const rhi::DescriptorReflection*>(MaterialAccess::reflection(*material));
        if (!reflection) return info;

        for (const auto& [range_name, range_info] : reflection->push_constant_ranges())
        {
            if (range_name.empty()) continue;

            PushConstantRangeRuntime runtime_range{};
            runtime_range.range_name = range_name;
            runtime_range.offset = range_info.offset;
            runtime_range.size = range_info.size;
            runtime_range.stages = range_info.stages;
            runtime_range.instancing_ssbo_name = range_name + "_instances";

            std::vector<const rhi::ShaderModule::PushConstantMember*> data_members{};
            const rhi::ShaderModule::PushConstantMember* use_instancing_member = nullptr;
            const rhi::ShaderModule::PushConstantMember* shadow_view_projection_member = nullptr;
            const rhi::ShaderModule::PushConstantMember* render_mode_member = nullptr;

            for (const auto& member : range_info.members)
            {
                if (member.name == instancing_field_name_)
                {
                    use_instancing_member = &member;
                    continue;
                }

                if (member.name == shadow_view_projection_field_name_)
                {
                    shadow_view_projection_member = &member;
                    continue;
                }

                if (member.name == render_mode_field_name_)
                {
                    render_mode_member = &member;
                    continue;
                }

                data_members.push_back(&member);
            }

            runtime_range.has_use_instancing_field = use_instancing_member != nullptr;
            if (use_instancing_member)
                runtime_range.use_instancing_offset_in_range = use_instancing_member->offset;

            runtime_range.has_shadow_view_projection_field = shadow_view_projection_member != nullptr;
            if (shadow_view_projection_member)
                runtime_range.shadow_view_projection_offset_in_range = shadow_view_projection_member->offset;

            runtime_range.has_render_mode_field = render_mode_member != nullptr;
            if (render_mode_member)
                runtime_range.render_mode_offset_in_range = render_mode_member->offset;

            if (data_members.size() == 1)
            {
                const auto* data_member = data_members.front();

                runtime_range.has_data_member = true;
                runtime_range.data_member_name = data_member->name;
                runtime_range.data_type_name = data_member->type_name;
                runtime_range.data_offset_in_range = data_member->offset;
                runtime_range.data_size = data_member->size;

                if (const auto struct_it = reflection->struct_types().find(data_member->type_name);
                    struct_it != reflection->struct_types().end())
                {
                    runtime_range.data_struct_size = struct_it->second.size;
                }
            }

            const std::string model_binding_name = range_name + "." + std::string(model_field_name_);
            if (const auto model_binding = reflection->lookup(model_binding_name);
                model_binding.has_value() &&
                model_binding->is_push_constant &&
                model_binding->size == sizeof(glm::mat4) &&
                is_within_range(model_binding->offset, model_binding->size, range_info.offset, range_info.size))
            {
                runtime_range.has_model_field = true;
                runtime_range.model_offset_in_range = model_binding->offset - range_info.offset;
            }

            const auto storage_it = reflection->storage_buffers().find(runtime_range.instancing_ssbo_name);

            const bool instancing_valid = validate_instancing(
                material, range_name, range_info, runtime_range,
                data_members, use_instancing_member, storage_it, reflection);

            runtime_range.instancing_supported = instancing_valid;

            if (runtime_range.instancing_supported && !info.instancing_range_index.has_value())
                info.instancing_range_index = info.ranges.size();

            info.ranges.push_back(std::move(runtime_range));
        }

        return info;
    }

    MaterialRenderInfo* get_or_build_render_info(Material* material)
    {
        if (!material) return nullptr;

        const auto it = material_render_infos_.find(material);
        if (it != material_render_infos_.end())
            return &it->second;

        auto [inserted_it, inserted] = material_render_infos_.try_emplace(
            material,
            build_material_render_info(material));

        if (!inserted && inserted_it == material_render_infos_.end()) return nullptr;
        return &inserted_it->second;
    }

    bool validate_instancing(
        [[maybe_unused]] Material* material,
        [[maybe_unused]] const std::string& range_name,
        [[maybe_unused]] const rhi::PushConstantRangeInfo& range_info,
        PushConstantRangeRuntime& runtime_range,
        [[maybe_unused]] const std::vector<const rhi::ShaderModule::PushConstantMember*>& data_members,
        [[maybe_unused]] const rhi::ShaderModule::PushConstantMember* use_instancing_member,
        const flat_map<std::string, rhi::ResourceInfo>::const_iterator& storage_it,
        const rhi::DescriptorReflection* reflection)
    {
        #ifdef BOZA_DEBUG
        const auto material_name = material->name();
        bool valid_layout = true;

        const std::size_t expected_member_count =
            1u +
            static_cast<std::size_t>(runtime_range.has_use_instancing_field) +
            static_cast<std::size_t>(runtime_range.has_shadow_view_projection_field) +
            static_cast<std::size_t>(runtime_range.has_render_mode_field);

        if (range_info.members.empty() || range_info.members.size() != expected_member_count)
        {
            Log::warn(
                "Material '{}' push constant range '{}' has unsupported fields for the engine push-constant contract",
                material_name, range_name);
            valid_layout = false;
        }

        if (data_members.size() != 1)
        {
            Log::warn(
                "Material '{}' push constant range '{}' must have exactly one data field T",
                material_name, range_name);
            valid_layout = false;
        }

        if (runtime_range.has_use_instancing_field)
        {
            if (range_name != "pc")
            {
                Log::warn(
                    "Material '{}' push constant range '{}' defines use_instancing, but only range 'pc' may use it",
                    material_name, range_name);
                valid_layout = false;
            }

            if (!use_instancing_member ||
                use_instancing_member->data_type != ShaderDataType::Uint ||
                use_instancing_member->size != sizeof(std::uint32_t))
            {
                Log::warn(
                    "Material '{}' push constant range '{}' field use_instancing must be uint",
                    material_name, range_name);
                valid_layout = false;
            }
        }

        if (runtime_range.has_shadow_view_projection_field)
        {
            const std::uint32_t offset = runtime_range.shadow_view_projection_offset_in_range;
            if (offset + sizeof(glm::mat4) > range_info.size)
            {
                Log::warn(
                    "Material '{}' push constant range '{}' field shadow_view_projection must be mat4",
                    material_name,
                    range_name);
                valid_layout = false;
            }
        }

        if (runtime_range.has_render_mode_field)
        {
            const std::uint32_t offset = runtime_range.render_mode_offset_in_range;
            if (offset + sizeof(std::uint32_t) > range_info.size)
            {
                Log::warn(
                    "Material '{}' push constant range '{}' field render_mode must be uint",
                    material_name,
                    range_name);
                valid_layout = false;
            }
        }

        if (!runtime_range.has_data_member)
        {
            valid_layout = false;
        }
        else
        {
            if (const auto* data_member = data_members.front();
                data_member->data_type != ShaderDataType::Unknown)
            {
                Log::warn(
                    "Material '{}' push constant range '{}' data field '{}' must be a struct type",
                    material_name, range_name, data_member->name);
                valid_layout = false;
            }

            if (runtime_range.data_type_name.empty() || runtime_range.data_struct_size == 0)
            {
                Log::warn(
                    "Material '{}' push constant range '{}' data type '{}' is missing valid struct reflection",
                    material_name, range_name, runtime_range.data_type_name);
                valid_layout = false;
            }
            else if (runtime_range.data_struct_size != runtime_range.data_size)
            {
                Log::warn(
                    "Material '{}' push constant range '{}' data field '{}' size {} does not match struct '{}' reflected size {}",
                    material_name, range_name,
                    runtime_range.data_member_name, runtime_range.data_size,
                    runtime_range.data_type_name, runtime_range.data_struct_size);
                valid_layout = false;
            }
        }

        bool instancing_valid =
            valid_layout &&
            runtime_range.has_use_instancing_field &&
            range_name == "pc";

        if (instancing_valid)
        {
            if (storage_it == reflection->storage_buffers().end())
            {
                Log::warn(
                    "Material '{}' requires storage buffer '{}' for instancing, but it was not found",
                    material_name, runtime_range.instancing_ssbo_name);
                instancing_valid = false;
            }
            else
            {
                const auto& storage = storage_it->second;

                if ((storage.stages & range_info.stages) != range_info.stages)
                {
                    Log::warn(
                        "Material '{}' storage buffer '{}' stage visibility does not cover push constant range '{}'",
                        material_name, runtime_range.instancing_ssbo_name, range_name);
                    instancing_valid = false;
                }

                if (storage.members.size() != 1)
                {
                    Log::warn(
                        "Material '{}' storage buffer '{}' must contain exactly one member (runtime array of '{}')",
                        material_name, runtime_range.instancing_ssbo_name, runtime_range.data_type_name);
                    instancing_valid = false;
                }
                else
                {
                    const auto& storage_member = storage.members.front();

                    if (!storage_member.is_runtime_array)
                    {
                        Log::warn(
                            "Material '{}' storage buffer '{}' member '{}' must be a runtime array",
                            material_name, runtime_range.instancing_ssbo_name, storage_member.name);
                        instancing_valid = false;
                    }

                    if (storage_member.type_name != runtime_range.data_type_name)
                    {
                        Log::warn(
                            "Material '{}' storage buffer '{}' member type '{}' must match push constant data type '{}'",
                            material_name, runtime_range.instancing_ssbo_name,
                            storage_member.type_name, runtime_range.data_type_name);
                        instancing_valid = false;
                    }

                    const std::uint32_t stride =
                        storage_member.array_stride > 0
                            ? storage_member.array_stride
                            : runtime_range.data_size;

                    if (stride == 0 || stride < runtime_range.data_size)
                    {
                        Log::warn(
                            "Material '{}' storage buffer '{}' has invalid array stride {} for type size {}",
                            material_name, runtime_range.instancing_ssbo_name,
                            stride, runtime_range.data_size);
                        instancing_valid = false;
                    }
                    else
                    {
                        runtime_range.instancing_stride = stride;
                    }
                }
            }
        }

        return instancing_valid;
        #else
        if (runtime_range.has_use_instancing_field &&
            range_name == "pc" &&
            runtime_range.has_data_member &&
            storage_it != reflection->storage_buffers().end() &&
            storage_it->second.members.size() == 1)
        {
            const auto& storage_member = storage_it->second.members.front();
            const std::uint32_t stride =
                storage_member.array_stride > 0
                    ? storage_member.array_stride
                    : runtime_range.data_size;

            runtime_range.instancing_stride = stride;
            return storage_member.is_runtime_array && stride >= runtime_range.data_size;
        }

        return false;
        #endif
    }

    void push_ranges(
        rhi::CommandBuffer* cmd,
        Material& material,
        const MaterialRenderInfo* render_info,
        const glm::mat4* model,
        const bool use_instancing,
        const glm::mat4* shadow_view_projection,
        const bool shadow_mode)
    {
        if (!render_info || render_info->ranges.empty()) return;

        auto* pipeline_layout = static_cast<rhi::PipelineLayout*>(MaterialAccess::pipeline_layout(material));
        if (!pipeline_layout) return;

        const std::span<const std::uint8_t> push_staging = MaterialAccess::push_constant_staging(material);

        for (const PushConstantRangeRuntime& range : render_info->ranges)
        {
            if (range.size == 0) continue;

            push_constant_scratch_.resize(range.size);

            if (range.offset < push_staging.size())
            {
                const std::size_t copy_size = std::min<std::size_t>(
                    range.size,
                    push_staging.size() - range.offset);

                std::memcpy(
                    push_constant_scratch_.data(),
                    push_staging.data() + range.offset,
                    copy_size);
            }

            if (model && range.has_model_field)
            {
                if (range.model_offset_in_range + sizeof(glm::mat4) <= push_constant_scratch_.size())
                {
                    std::memcpy(
                        push_constant_scratch_.data() + range.model_offset_in_range,
                        model,
                        sizeof(glm::mat4));
                }
            }

            if (range.has_use_instancing_field)
            {
                constexpr std::size_t use_instancing_size = sizeof(std::uint32_t);
                if (range.use_instancing_offset_in_range + use_instancing_size <= push_constant_scratch_.size())
                {
                    const std::uint32_t value = use_instancing ? 1u : 0u;
                    std::memcpy(
                        push_constant_scratch_.data() + range.use_instancing_offset_in_range,
                        &value,
                        use_instancing_size);
                }
            }

            if (shadow_view_projection && range.has_shadow_view_projection_field)
            {
                if (range.shadow_view_projection_offset_in_range + sizeof(glm::mat4) <= push_constant_scratch_.size())
                {
                    std::memcpy(
                        push_constant_scratch_.data() + range.shadow_view_projection_offset_in_range,
                        shadow_view_projection,
                        sizeof(glm::mat4));
                }
            }

            if (range.has_render_mode_field)
            {
                constexpr std::size_t render_mode_size = sizeof(std::uint32_t);
                if (range.render_mode_offset_in_range + render_mode_size <= push_constant_scratch_.size())
                {
                    const std::uint32_t render_mode = shadow_mode ? 1u : 0u;
                    std::memcpy(
                        push_constant_scratch_.data() + range.render_mode_offset_in_range,
                        &render_mode,
                        render_mode_size);
                }
            }

            const auto stage_bits = range.stages.value();
            if (stage_bits == 0) continue;

            cmd->push_constants(
                pipeline_layout,
                static_cast<rhi::ShaderStage>(stage_bits),
                range.offset,
                range.size,
                push_constant_scratch_.data());
        }
    }

    bool upload_to_instance_buffer(
        Material* material,
        const std::string& ssbo_name,
        const std::size_t actual_payload_size)
    {
        InstanceBufferState* buffer_state = get_instance_buffer(material, ssbo_name);

        if (!buffer_state ||
            !ensure_buffer_capacity(*buffer_state, actual_payload_size) ||
            !buffer_state->buffer)
            return false;

        buffer_state->buffer->upload(instance_payload_scratch_.data(), actual_payload_size, 0);

        const auto buffer_handle = BufferAccess::handle(*buffer_state->buffer);
        if (buffer_state->bound_handle != buffer_handle)
        {
            material->update_buffer(ssbo_name, *buffer_state->buffer);
            buffer_state->bound_handle = buffer_handle;
        }

        return true;
    }

    bool upload_instance_payload_from_models(
        Material* material,
        const std::span<const glm::mat4> models,
        const PushConstantRangeRuntime& range)
    {
        if (!material || models.empty()) return false;

        if (!range.instancing_supported ||
            !range.has_data_member ||
            range.data_size == 0 ||
            range.instancing_stride == 0 ||
            range.instancing_stride < range.data_size)
            return false;

        const std::size_t payload_size = models.size() * static_cast<std::size_t>(range.instancing_stride);
        if (payload_size == 0) return false;

        instance_payload_scratch_.resize(payload_size);

        const std::span<const std::uint8_t> push_staging = MaterialAccess::push_constant_staging(*material);

        const std::size_t data_base_offset =
            static_cast<std::size_t>(range.offset) +
            static_cast<std::size_t>(range.data_offset_in_range);

        std::size_t base_copy_size = 0;
        if (data_base_offset < push_staging.size())
            base_copy_size = std::min<std::size_t>(range.data_size, push_staging.size() - data_base_offset);

        std::size_t model_offset_in_data = 0;
        bool can_write_model = false;
        if (range.has_model_field && range.model_offset_in_range >= range.data_offset_in_range)
        {
            model_offset_in_data = range.model_offset_in_range - range.data_offset_in_range;
            can_write_model = model_offset_in_data + sizeof(glm::mat4) <= range.data_size;
        }

        if (!can_write_model) return false;

        const bool matrix_only_payload =
            model_offset_in_data == 0 &&
            range.data_size == sizeof(glm::mat4) &&
            range.instancing_stride == sizeof(glm::mat4);

        for (std::size_t i = 0; i < models.size(); ++i)
        {
            std::uint8_t* dst = instance_payload_scratch_.data() + i * static_cast<std::size_t>(range.instancing_stride);

            if (base_copy_size > 0 && !matrix_only_payload)
                std::memcpy(dst, push_staging.data() + data_base_offset, base_copy_size);

            std::memcpy(dst + model_offset_in_data, &models[i], sizeof(glm::mat4));
        }

        return upload_to_instance_buffer(material, range.instancing_ssbo_name, payload_size);
    }

    const PushConstantRangeRuntime* select_fallback_range(
        [[maybe_unused]] Material& material,
        const MaterialRenderInfo* render_info)
    {
        if (render_info->instancing_range_index.has_value())
        {
            const std::size_t range_index = *render_info->instancing_range_index;
            if (range_index < render_info->ranges.size())
                return &render_info->ranges[range_index];

            #ifdef BOZA_DEBUG
            Log::warn(
                "Material '{}' has invalid instancing range index {} (range count: {})",
                material.name(), range_index, render_info->ranges.size());
            #endif
        }

        for (const PushConstantRangeRuntime& candidate : render_info->ranges)
        {
            if (!candidate.has_use_instancing_field) continue;

            const auto binding = material.lookup_binding(candidate.instancing_ssbo_name);
            if (!binding.has_value()) continue;

            if (binding->descriptor_type != static_cast<std::uint32_t>(rhi::DescriptorType::StorageBuffer))
                continue;

            return &candidate;
        }

        return nullptr;
    }

    void bind_fallback_ssbo(
        Material& material,
        const PushConstantRangeRuntime& range)
    {
        const auto binding = material.lookup_binding(range.instancing_ssbo_name);
        if (!binding.has_value()) return;

        if (binding->descriptor_type != static_cast<std::uint32_t>(rhi::DescriptorType::StorageBuffer))
            return;

        InstanceBufferState* buffer_state =
            get_instance_buffer(&material, range.instancing_ssbo_name);
        if (!buffer_state) return;

        const std::size_t fallback_stride =
            range.data_size > 0
                ? static_cast<std::size_t>(range.data_size)
                : sizeof(glm::mat4);

        const std::size_t stride = std::max<std::size_t>(
            static_cast<std::size_t>(range.instancing_stride),
            std::max<std::size_t>(fallback_stride, 1));

        if (!ensure_buffer_capacity(*buffer_state, stride) || !buffer_state->buffer)
            return;

        const auto buffer_handle = BufferAccess::handle(*buffer_state->buffer);
        if (buffer_state->bound_handle == buffer_handle)
            return;

        instance_payload_scratch_.resize(stride);
        std::fill(instance_payload_scratch_.begin(), instance_payload_scratch_.end(), std::uint8_t{ 0 });

        if (range.has_model_field && range.model_offset_in_range >= range.data_offset_in_range)
        {
            const std::size_t model_offset_in_data = range.model_offset_in_range - range.data_offset_in_range;
            if (model_offset_in_data + sizeof(glm::mat4) <= instance_payload_scratch_.size())
            {
                const glm::mat4 identity = glm::identity<glm::mat4>();
                std::memcpy(
                    instance_payload_scratch_.data() + model_offset_in_data,
                    &identity,
                    sizeof(glm::mat4));
            }
        }

        buffer_state->buffer->upload(instance_payload_scratch_.data(), stride, 0);

        material.update_buffer(range.instancing_ssbo_name, *buffer_state->buffer);
        buffer_state->bound_handle = buffer_handle;
    }

    void ensure_fallback_ssbo_bound(Material& material, const MaterialRenderInfo* render_info)
    {
        if (!render_info) return;

        const PushConstantRangeRuntime* selected_range = select_fallback_range(material, render_info);
        if (!selected_range) return;

        bind_fallback_ssbo(material, *selected_range);
    }

    void bind_frame_render_resources(Material& material)
    {
        auto& material_loader = gfx::MaterialLoader::instance();

        if (Texture* directional_shadow_map = material_loader.directional_shadow_map();
            directional_shadow_map &&
            material_loader.directional_shadow_sampler() &&
            material.lookup_binding("directional_shadow_map").has_value())
        {
            material.update_texture(
                "directional_shadow_map",
                *directional_shadow_map,
                *material_loader.directional_shadow_sampler());
        }

        if (Texture* point_shadow_maps = material_loader.point_shadow_map();
            point_shadow_maps &&
            material_loader.point_shadow_sampler() &&
            material.lookup_binding("point_shadow_maps").has_value())
        {
            material.update_texture(
                "point_shadow_maps",
                *point_shadow_maps,
                *material_loader.point_shadow_sampler());
        }

        if (Texture* spot_shadow_maps = material_loader.spot_shadow_map();
            spot_shadow_maps &&
            material_loader.spot_shadow_sampler() &&
            material.lookup_binding("spot_shadow_maps").has_value())
        {
            material.update_texture(
                "spot_shadow_maps",
                *spot_shadow_maps,
                *material_loader.spot_shadow_sampler());
        }

        if (Texture* ssao_texture = material_loader.ssao_texture();
            ssao_texture &&
            material_loader.renderer_sampler() &&
            material.lookup_binding("ssao_texture").has_value())
        {
            material.update_texture(
                "ssao_texture",
                *ssao_texture,
                *material_loader.renderer_sampler());
        }
    }

    void RenderingSystem::on_mesh_destroyed(Mesh* mesh)
    {
        assert_render_thread();

        if (!mesh) return;

        destroyed_meshes_this_frame_.insert(mesh);

        valid_meshes_.erase(mesh);
        invalid_meshes_.insert(mesh);
    }

    void RenderingSystem::on_material_destroyed(Material* material)
    {
        assert_render_thread();

        if (!material) return;

        destroyed_materials_this_frame_.insert(material);

        valid_materials_.erase(material);
        invalid_materials_.insert(material);
    }
}
