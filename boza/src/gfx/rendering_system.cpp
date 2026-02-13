module boza.gfx;

import :rendering_system;

import boza.core;
import boza.gfx.material_loader;
import boza.gfx.texture_loader;
import boza.gfx.sampler_loader;

import boza.platform;
import boza.rhi;
import boza.rhi.render_context;
import boza.app.game_settings;

namespace boza
{
    namespace
    {
        constexpr std::size_t mesh_material_instancing_threshold = 16;
        constexpr std::size_t initial_instance_buffer_capacity_bytes =
            65'536 * sizeof(glm::mat4);

        constexpr std::string_view engine_model_field_name = "model";
        constexpr std::string_view engine_instancing_field_name = "use_instancing";

        struct FrustumPlane final
        {
            glm::vec3 normal{ 0.0f };
            float distance{ 0.0f };
        };

        struct MeshMaterialKey final
        {
            Mesh* mesh{ nullptr };
            Material* material{ nullptr };

            bool operator==(const MeshMaterialKey&) const = default;
        };

        struct MeshMaterialKeyHash final
        {
            std::size_t operator()(const MeshMaterialKey& key) const noexcept
            {
                const std::size_t mesh_hash     = std::hash<Mesh*>{}(key.mesh);
                const std::size_t material_hash = std::hash<Material*>{}(key.material);
                return mesh_hash ^ (material_hash << 1);
            }
        };

        struct DrawGroup final
        {
            Mesh* mesh{ nullptr };
            Material* material{ nullptr };
            std::vector<glm::mat4> world_matrices{};
            std::size_t active_instance_count{ 0 };
        };

        struct PushConstantRangeRuntime final
        {
            std::string range_name{};
            std::uint32_t offset{ 0 };
            std::uint32_t size{ 0 };
            Flags<rhi::ShaderStage> stages{};

            bool has_model_field{ false };
            std::uint32_t model_offset_in_range{ 0 };

            bool has_use_instancing_field{ false };
            std::uint32_t use_instancing_offset_in_range{ 0 };

            bool has_data_member{ false };
            std::string data_member_name{};
            std::string data_type_name{};
            std::uint32_t data_offset_in_range{ 0 };
            std::uint32_t data_size{ 0 };
            std::uint32_t data_struct_size{ 0 };

            bool instancing_supported{ false };
            std::string instancing_storage_buffer_name{};
            std::uint32_t instancing_stride{ 0 };
        };

        struct MaterialRenderInfo final
        {
            std::vector<PushConstantRangeRuntime> ranges{};
            std::optional<std::size_t> instancing_range_index{};
        };

        struct MaterialInstanceBufferState final
        {
            std::unique_ptr<Buffer> buffer{ nullptr };
            std::size_t capacity_bytes{ 0 };
            std::vector<void*> bound_buffer_handles{};
            std::vector<void*> mapped_frame_data{};
        };

        struct FrustumState final
        {
            static FrustumPlane normalize_plane(const glm::vec4& plane)
            {
                const glm::vec3 normal{ plane.x, plane.y, plane.z };
                const float normal_length = glm::length(normal);

                if (normal_length <= std::numeric_limits<float>::epsilon()) return {};

                return {
                    .normal = normal / normal_length,
                    .distance = plane.w / normal_length
                };
            }

            void set_from_view_projection(const glm::mat4& view_projection)
            {
                const glm::mat4 m = transpose(view_projection);

                planes[0] = normalize_plane(m[3] + m[0]); // left
                planes[1] = normalize_plane(m[3] - m[0]); // right
                planes[2] = normalize_plane(m[3] + m[1]); // bottom
                planes[3] = normalize_plane(m[3] - m[1]); // top
                planes[4] = normalize_plane(m[3] + m[2]); // near
                planes[5] = normalize_plane(m[3] - m[2]); // far

                valid = true;
            }

            [[nodiscard]] bool sphere_visible(const glm::vec3& center, const float radius) const
            {
                if (!valid) return true;

                for (const FrustumPlane& plane : planes)
                {
                    const float signed_distance = dot(plane.normal, center) + plane.distance;
                    if (signed_distance < -radius) return false;
                }

                return true;
            }

            bool valid{ false };
            std::array<FrustumPlane, 6> planes{};
        };
        FrustumState frustum_state{};

        node_map<MeshMaterialKey, DrawGroup, MeshMaterialKeyHash> draw_groups{};
        std::vector<MeshMaterialKey>                               draw_group_order{};
        flat_map<Material*, MaterialRenderInfo> material_render_infos{};
        flat_map<Material*, flat_map<std::string, MaterialInstanceBufferState>> material_instance_buffers{};

        std::vector<std::uint8_t> push_constant_range_data{};
        std::vector<std::uint8_t> instance_payload_data{};
        bool frame_render_active{ false };

        void reset_frame_draw_state()
        {
            for (const MeshMaterialKey& key : draw_group_order)
            {
                if (const auto it = draw_groups.find(key); it != draw_groups.end())
                {
                    it->second.active_instance_count = 0;
                }
            }

            draw_group_order.clear();
        }

        void reset_render_caches()
        {
            for (auto& per_binding : material_instance_buffers | std::views::values)
            {
                for (auto& state : per_binding | std::views::values)
                {
                    if (!state.buffer) continue;

                    const std::size_t mapped_count = state.mapped_frame_data.size();
                    for (std::size_t frame_index = 0; frame_index < mapped_count; ++frame_index)
                    {
                        if (state.mapped_frame_data[frame_index]) state.buffer->unmap(static_cast<std::uint32_t>(frame_index));
                    }

                    state.mapped_frame_data.clear();
                    state.bound_buffer_handles.clear();
                }
            }

            material_render_infos.clear();
            material_instance_buffers.clear();
            push_constant_range_data.clear();
            instance_payload_data.clear();
        }

        [[nodiscard]] std::uint32_t frame_slot_count()
        {
            return std::max<std::uint32_t>(rhi::RenderContext::frames_in_flight(), 1u);
        }

        [[nodiscard]] std::uint32_t current_frame_index()
        {
            if (const auto* swapchain = rhi::RenderContext::swapchain())
            {
                return swapchain->current_frame();
            }

            return 0;
        }

        [[nodiscard]] MaterialInstanceBufferState* get_material_instance_buffer_state(
            Material* material,
            const std::string& binding_name)
        {
            if (!material) return nullptr;

            auto& per_binding = material_instance_buffers[material];
            if (!per_binding.contains(binding_name))
            {
                per_binding.emplace(binding_name, MaterialInstanceBufferState{});
            }

            auto it = per_binding.find(binding_name);
            if (it == per_binding.end()) return nullptr;
            return &it->second;
        }

        bool ensure_instance_buffer_capacity(
            MaterialInstanceBufferState& state,
            const std::size_t required_bytes)
        {
            if (required_bytes == 0) return true;
            if (state.buffer && state.capacity_bytes >= required_bytes) return true;

            if (state.buffer)
            {
                const std::size_t mapped_count = state.mapped_frame_data.size();
                for (std::size_t frame_index = 0; frame_index < mapped_count; ++frame_index)
                {
                    if (state.mapped_frame_data[frame_index]) state.buffer->unmap(static_cast<std::uint32_t>(frame_index));
                }

                state.mapped_frame_data.clear();
                state.bound_buffer_handles.clear();
            }

            const std::size_t doubled_capacity =
                state.capacity_bytes > 0
                    ? state.capacity_bytes * 2
                    : initial_instance_buffer_capacity_bytes;

            const std::size_t new_capacity = std::max({
                initial_instance_buffer_capacity_bytes,
                doubled_capacity,
                required_bytes
            });

            auto new_buffer = std::make_unique<Buffer>(
                new_capacity,
                BufferUsage::Storage,
                ResourceAccessMode::Dynamic);

            if (!new_buffer->rhi_handle(0))
            {
                Log::error("Failed to allocate instance payload buffer");
                return false;
            }

            const std::uint32_t slot_count = frame_slot_count();
            std::vector<void*> mapped_frame_data(slot_count, nullptr);

            for (std::uint32_t frame_index = 0; frame_index < slot_count; ++frame_index)
            {
                mapped_frame_data[frame_index] = new_buffer->map(frame_index);

                if (!mapped_frame_data[frame_index])
                {
                    for (std::uint32_t cleanup_index = 0; cleanup_index < frame_index; ++cleanup_index)
                    {
                        if (mapped_frame_data[cleanup_index]) new_buffer->unmap(cleanup_index);
                    }

                    Log::error("Failed to map instance payload buffer for frame {}", frame_index);
                    return false;
                }
            }

            state.buffer = std::move(new_buffer);
            state.capacity_bytes = new_capacity;
            state.bound_buffer_handles.assign(slot_count, nullptr);
            state.mapped_frame_data = std::move(mapped_frame_data);
            return true;
        }

        [[nodiscard]] bool is_within_range(
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

        [[nodiscard]] MaterialRenderInfo build_material_render_info(Material* material)
        {
            MaterialRenderInfo info{};
            if (!material) return info;

            const auto* reflection = static_cast<const rhi::DescriptorReflection*>(material->reflection_handle());
            if (!reflection) return info;

            for (const auto& [range_name, range_info] : reflection->push_constant_ranges())
            {
                if (range_name.empty()) continue;

                PushConstantRangeRuntime runtime_range{};
                runtime_range.range_name = range_name;
                runtime_range.offset = range_info.offset;
                runtime_range.size = range_info.size;
                runtime_range.stages = range_info.stages;
                runtime_range.instancing_storage_buffer_name = range_name + "_instances";

                std::vector<const rhi::ShaderModule::PushConstantMember*> data_members{};
                const rhi::ShaderModule::PushConstantMember* use_instancing_member = nullptr;

                for (const auto& member : range_info.members)
                {
                    if (member.name == engine_instancing_field_name)
                    {
                        use_instancing_member = &member;
                        continue;
                    }

                    data_members.push_back(&member);
                }

                runtime_range.has_use_instancing_field = use_instancing_member != nullptr;
                if (use_instancing_member)
                {
                    runtime_range.use_instancing_offset_in_range = use_instancing_member->offset;
                }

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

                const std::string model_binding_name = range_name + "." + std::string(engine_model_field_name);
                if (const auto model_binding = reflection->lookup(model_binding_name);
                    model_binding.has_value() &&
                    model_binding->is_push_constant &&
                    model_binding->size == sizeof(glm::mat4) &&
                    is_within_range(model_binding->offset, model_binding->size, range_info.offset, range_info.size))
                {
                    runtime_range.has_model_field = true;
                    runtime_range.model_offset_in_range = model_binding->offset - range_info.offset;
                }

                const auto storage_it = reflection->storage_buffers().find(runtime_range.instancing_storage_buffer_name);

                #ifdef BOZA_DEBUG
                const auto material_name = material->name();
                bool valid_layout = true;

                if (range_info.members.empty() || range_info.members.size() > 2)
                {
                    Log::warn(
                        "Material '{}' push constant range '{}' must contain only one data field T and optional uint use_instancing",
                        material_name,
                        range_name);
                    valid_layout = false;
                }

                if (data_members.size() != 1)
                {
                    Log::warn(
                        "Material '{}' push constant range '{}' must have exactly one data field T",
                        material_name,
                        range_name);
                    valid_layout = false;
                }

                if (!runtime_range.has_use_instancing_field && range_info.members.size() != 1)
                {
                    Log::warn(
                        "Material '{}' push constant range '{}' has unsupported fields; expected exactly one data field",
                        material_name,
                        range_name);
                    valid_layout = false;
                }

                if (runtime_range.has_use_instancing_field)
                {
                    if (range_info.members.size() != 2)
                    {
                        Log::warn(
                            "Material '{}' push constant range '{}' has unsupported fields; expected data field + uint use_instancing",
                            material_name,
                            range_name);
                        valid_layout = false;
                    }

                    if (range_name != "pc")
                    {
                        Log::warn(
                            "Material '{}' push constant range '{}' defines use_instancing, but only range 'pc' may use it",
                            material_name,
                            range_name);
                        valid_layout = false;
                    }

                    if (!use_instancing_member ||
                        use_instancing_member->data_type != ShaderDataType::Uint ||
                        use_instancing_member->size != sizeof(std::uint32_t))
                    {
                        Log::warn(
                            "Material '{}' push constant range '{}' field use_instancing must be uint",
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
                            material_name,
                            range_name,
                            data_member->name);
                        valid_layout = false;
                    }

                    if (runtime_range.data_type_name.empty() || runtime_range.data_struct_size == 0)
                    {
                        Log::warn(
                            "Material '{}' push constant range '{}' data type '{}' is missing valid struct reflection",
                            material_name,
                            range_name,
                            runtime_range.data_type_name);
                        valid_layout = false;
                    }
                    else if (runtime_range.data_struct_size != runtime_range.data_size)
                    {
                        Log::warn(
                            "Material '{}' push constant range '{}' data field '{}' size {} does not match struct '{}' reflected size {}",
                            material_name,
                            range_name,
                            runtime_range.data_member_name,
                            runtime_range.data_size,
                            runtime_range.data_type_name,
                            runtime_range.data_struct_size);
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
                            material_name,
                            runtime_range.instancing_storage_buffer_name);
                        instancing_valid = false;
                    }
                    else
                    {
                        const auto& storage = storage_it->second;

                        const auto range_stages = range_info.stages.value();
                        const auto storage_stages = storage.stages.value();
                        if ((storage_stages & range_stages) != range_stages)
                        {
                            Log::warn(
                                "Material '{}' storage buffer '{}' stage visibility does not cover push constant range '{}'",
                                material_name,
                                runtime_range.instancing_storage_buffer_name,
                                range_name);
                            instancing_valid = false;
                        }

                        if (storage.members.size() != 1)
                        {
                            Log::warn(
                                "Material '{}' storage buffer '{}' must contain exactly one member (runtime array of '{}')",
                                material_name,
                                runtime_range.instancing_storage_buffer_name,
                                runtime_range.data_type_name);
                            instancing_valid = false;
                        }
                        else
                        {
                            const auto& storage_member = storage.members.front();

                            if (!storage_member.is_runtime_array)
                            {
                                Log::warn(
                                    "Material '{}' storage buffer '{}' member '{}' must be a runtime array",
                                    material_name,
                                    runtime_range.instancing_storage_buffer_name,
                                    storage_member.name);
                                instancing_valid = false;
                            }

                            if (storage_member.type_name != runtime_range.data_type_name)
                            {
                                Log::warn(
                                    "Material '{}' storage buffer '{}' member type '{}' must match push constant data type '{}'",
                                    material_name,
                                    runtime_range.instancing_storage_buffer_name,
                                    storage_member.type_name,
                                    runtime_range.data_type_name);
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
                                    material_name,
                                    runtime_range.instancing_storage_buffer_name,
                                    stride,
                                    runtime_range.data_size);
                                instancing_valid = false;
                            }
                            else
                            {
                                runtime_range.instancing_stride = stride;
                            }
                        }
                    }
                }

                runtime_range.instancing_supported = instancing_valid;
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
                    runtime_range.instancing_supported =
                        storage_member.is_runtime_array &&
                        stride >= runtime_range.data_size;
                }
                #endif

                if (runtime_range.instancing_supported && !info.instancing_range_index.has_value())
                {
                    info.instancing_range_index = info.ranges.size();
                }

                info.ranges.push_back(std::move(runtime_range));
            }

            return info;
        }

        [[nodiscard]] MaterialRenderInfo* get_material_render_info(Material* material)
        {
            if (!material) return nullptr;

            const auto it = material_render_infos.find(material);
            if (it != material_render_infos.end())
            {
                return &it->second;
            }

            auto [inserted_it, inserted] = material_render_infos.try_emplace(
                material,
                build_material_render_info(material));

            if (!inserted && inserted_it == material_render_infos.end()) return nullptr;
            return &inserted_it->second;
        }
    }

    std::unique_ptr<rhi::Instance>       RenderingSystem::instance_{ nullptr };
    std::unique_ptr<rhi::Device>         RenderingSystem::device_{ nullptr };
    std::unique_ptr<rhi::Swapchain>      RenderingSystem::swapchain_{ nullptr };
    std::unique_ptr<rhi::DescriptorPool> RenderingSystem::descriptor_pool_{ nullptr };
    std::unique_ptr<rhi::ResourceCache>  RenderingSystem::resource_cache_{ nullptr };


    void RenderingSystem::EngineBegin::execute()
    {
        if (rhi::RenderContext::initialized()) return;

        frustum_state.valid = false;

        if (!init_graphics())
        {
            Log::critical("Failed to initialize graphics");
            return;
        }

        setup_resources();
    }


    void RenderingSystem::BeginFrame::execute()
    {
        frame_render_active = false;

        reset_frame_draw_state();

        if (!swapchain_)
        {
            rhi::RenderContext::set_current_command_buffer(nullptr);
            return;
        }

        if (!swapchain_->begin_frame())
        {
            rhi::RenderContext::set_current_command_buffer(nullptr);
            return;
        }

        const std::uint32_t image_idx = swapchain_->current_image_index();
        rhi::CommandBuffer* cmd = swapchain_->current_command_buffer();

        rhi::RenderContext::set_current_command_buffer(cmd);

        const float dt = Time::delta_time();
        static float accumulated_time = 0.0f;
        accumulated_time += dt;
        gfx::MaterialLoader::instance().update_time_ubo(accumulated_time, dt);

        swapchain_->begin_render_pass(image_idx);
        frame_render_active = true;
    }

    void RenderingSystem::EndFrame::execute()
    {
        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!swapchain_ || !cmd)
        {
            frame_render_active = false;
            reset_frame_draw_state();
            return;
        }

        struct DrawPlan final
        {
            DrawGroup* group{ nullptr };
            bool use_instancing{ false };
        };

        std::vector<DrawPlan> plans{};
        plans.reserve(draw_group_order.size());

        constexpr std::size_t max_instance_index = std::numeric_limits<std::uint32_t>::max();

        for (const MeshMaterialKey& key : draw_group_order)
        {
            const auto it = draw_groups.find(key);
            if (it == draw_groups.end()) continue;

            DrawGroup& group = it->second;
            if (!group.mesh || !group.material || group.active_instance_count == 0) continue;

            MaterialRenderInfo* render_info = get_material_render_info(group.material);

            const bool should_instance =
                render_info &&
                render_info->instancing_range_index.has_value() &&
                group.active_instance_count >= mesh_material_instancing_threshold &&
                group.active_instance_count <= max_instance_index;

            plans.push_back(
                DrawPlan{
                    .group = &group,
                    .use_instancing = should_instance
                });
        }

        const auto push_ranges_for_draw = [cmd](
            Material& material,
            const MaterialRenderInfo* render_info,
            const glm::mat4* model_matrix,
            const bool use_instancing)
        {
            if (!render_info || render_info->ranges.empty()) return;

            auto* pipeline_layout = static_cast<rhi::PipelineLayout*>(material.rhi_pipeline_layout_handle());
            if (!pipeline_layout) return;

            const std::span<const std::uint8_t> push_staging = material.push_constant_staging();

            for (const PushConstantRangeRuntime& range : render_info->ranges)
            {
                if (range.size == 0) continue;

                push_constant_range_data.resize(range.size);

                if (range.offset < push_staging.size())
                {
                    const std::size_t copy_size = std::min<std::size_t>(
                        range.size,
                        push_staging.size() - range.offset);

                    std::memcpy(
                        push_constant_range_data.data(),
                        push_staging.data() + range.offset,
                        copy_size);
                }

                if (model_matrix && range.has_model_field)
                {
                    if (range.model_offset_in_range + sizeof(glm::mat4) <= push_constant_range_data.size())
                    {
                        std::memcpy(
                            push_constant_range_data.data() + range.model_offset_in_range,
                            model_matrix,
                            sizeof(glm::mat4));
                    }
                }

                if (range.has_use_instancing_field)
                {
                    constexpr std::size_t use_instancing_size = sizeof(std::uint32_t);
                    if (range.use_instancing_offset_in_range + use_instancing_size <= push_constant_range_data.size())
                    {
                        const std::uint32_t value = use_instancing ? 1u : 0u;
                        std::memcpy(
                            push_constant_range_data.data() + range.use_instancing_offset_in_range,
                            &value,
                            use_instancing_size);
                    }
                }

                const auto stage_bits = range.stages.value();
                if (stage_bits == 0) continue;

                cmd->push_constants(
                    pipeline_layout,
                    static_cast<rhi::ShaderStage>(stage_bits),
                    range.offset,
                    range.size,
                    push_constant_range_data.data());
            }
        };

        const auto upload_instance_payload_for_plan = [](const DrawGroup& group, const MaterialRenderInfo* render_info) -> bool
        {
            if (!group.material ||
                !render_info ||
                !render_info->instancing_range_index.has_value())
                return false;

            const std::size_t range_index = *render_info->instancing_range_index;
            if (range_index >= render_info->ranges.size())
            {
                #ifdef BOZA_DEBUG
                Log::warn(
                    "Material '{}' has invalid instancing range index {} (range count: {})",
                    group.material->name(),
                    range_index,
                    render_info->ranges.size());
                #endif
                return false;
            }

            const PushConstantRangeRuntime& range = render_info->ranges[range_index];

            if (!range.instancing_supported ||
                !range.has_data_member ||
                range.data_size == 0 ||
                range.instancing_stride == 0 ||
                range.instancing_stride < range.data_size)
                return false;

            const std::size_t instance_count = group.active_instance_count;
            if (instance_count == 0 ||
                instance_count > std::numeric_limits<std::uint32_t>::max()) return false;

            const std::size_t payload_size = instance_count * static_cast<std::size_t>(range.instancing_stride);
            if (payload_size == 0) return false;

            instance_payload_data.resize(payload_size);

            const std::span<const std::uint8_t> push_staging = group.material->push_constant_staging();

            const std::size_t data_base_offset =
                static_cast<std::size_t>(range.offset) +
                static_cast<std::size_t>(range.data_offset_in_range);

            std::size_t base_copy_size = 0;
            if (data_base_offset < push_staging.size())
            {
                base_copy_size = std::min<std::size_t>(range.data_size, push_staging.size() - data_base_offset);
            }

            std::size_t model_offset_in_data = 0;
            bool can_write_model = false;
            if (range.has_model_field && range.model_offset_in_range >= range.data_offset_in_range)
            {
                model_offset_in_data = range.model_offset_in_range - range.data_offset_in_range;
                can_write_model = model_offset_in_data + sizeof(glm::mat4) <= range.data_size;
            }

            const bool matrix_only_payload =
                can_write_model &&
                model_offset_in_data == 0 &&
                range.data_size == sizeof(glm::mat4) &&
                range.instancing_stride == sizeof(glm::mat4);

            if (matrix_only_payload)
            {
                std::memcpy(
                    instance_payload_data.data(),
                    group.world_matrices.data(),
                    instance_count * sizeof(glm::mat4));
            }
            else
            {
                const bool base_copy_redundant =
                    can_write_model &&
                    model_offset_in_data == 0 &&
                    base_copy_size <= sizeof(glm::mat4);

                for (std::size_t instance_index = 0; instance_index < instance_count; ++instance_index)
                {
                    std::uint8_t* dst =
                        instance_payload_data.data() +
                        instance_index * static_cast<std::size_t>(range.instancing_stride);

                    if (base_copy_size > 0 && !base_copy_redundant)
                    {
                        std::memcpy(dst, push_staging.data() + data_base_offset, base_copy_size);
                    }

                    if (can_write_model)
                    {
                        std::memcpy(
                            dst + model_offset_in_data,
                            &group.world_matrices[instance_index],
                            sizeof(glm::mat4));
                    }
                }
            }

            MaterialInstanceBufferState* buffer_state = get_material_instance_buffer_state(
                group.material,
                range.instancing_storage_buffer_name);

            if (!buffer_state ||
                !ensure_instance_buffer_capacity(*buffer_state, payload_size) ||
                !buffer_state->buffer)
                return false;

            const std::uint32_t frame_index = current_frame_index();

            if (frame_index >= buffer_state->mapped_frame_data.size() ||
                frame_index >= buffer_state->bound_buffer_handles.size())
            {
                return false;
            }

            if (void* mapped = buffer_state->mapped_frame_data[frame_index])
            {
                std::memcpy(mapped, instance_payload_data.data(), payload_size);
            }
            else
            {
                buffer_state->buffer->upload(instance_payload_data.data(), payload_size, 0, frame_index);
            }

            const auto buffer_handle = buffer_state->buffer->rhi_handle(frame_index);
            if (buffer_state->bound_buffer_handles[frame_index] != buffer_handle)
            {
                group.material->update_buffer(
                    range.instancing_storage_buffer_name,
                    *buffer_state->buffer);
                buffer_state->bound_buffer_handles[frame_index] = buffer_handle;
            }

            return true;
        };

        const auto ensure_instancing_descriptor_bound = [](Material& material, const MaterialRenderInfo* render_info) -> bool
        {
            if (!render_info) return true;

            const PushConstantRangeRuntime* selected_range = nullptr;

            if (render_info->instancing_range_index.has_value())
            {
                const std::size_t range_index = *render_info->instancing_range_index;
                if (range_index < render_info->ranges.size())
                {
                    selected_range = &render_info->ranges[range_index];
                }
                else
                {
                    #ifdef BOZA_DEBUG
                    Log::warn(
                        "Material '{}' has invalid instancing range index {} (range count: {})",
                        material.name(),
                        range_index,
                        render_info->ranges.size());
                    #endif
                }
            }

            if (!selected_range)
            {
                for (const PushConstantRangeRuntime& candidate : render_info->ranges)
                {
                    if (!candidate.has_use_instancing_field) continue;

                    const auto binding = material.lookup_binding(candidate.instancing_storage_buffer_name);
                    if (!binding.has_value()) continue;

                    if (binding->descriptor_type != static_cast<std::uint32_t>(rhi::DescriptorType::StorageBuffer))
                    {
                        continue;
                    }

                    selected_range = &candidate;
                    break;
                }
            }

            if (!selected_range) return true;

            const PushConstantRangeRuntime& range = *selected_range;

            const auto binding = material.lookup_binding(range.instancing_storage_buffer_name);
            if (!binding.has_value()) return true;

            if (binding->descriptor_type != static_cast<std::uint32_t>(rhi::DescriptorType::StorageBuffer))
            {
                return true;
            }

            MaterialInstanceBufferState* buffer_state =
                get_material_instance_buffer_state(&material, range.instancing_storage_buffer_name);
            if (!buffer_state) return false;

            const std::size_t fallback_stride =
                range.data_size > 0
                    ? static_cast<std::size_t>(range.data_size)
                    : sizeof(glm::mat4);

            const std::size_t stride = std::max<std::size_t>(
                static_cast<std::size_t>(range.instancing_stride),
                std::max<std::size_t>(fallback_stride, 1));

            if (!ensure_instance_buffer_capacity(*buffer_state, stride) || !buffer_state->buffer)
            {
                return false;
            }

            const std::uint32_t frame_index = current_frame_index();

            if (frame_index >= buffer_state->mapped_frame_data.size() ||
                frame_index >= buffer_state->bound_buffer_handles.size())
            {
                return false;
            }

            const auto buffer_handle = buffer_state->buffer->rhi_handle(frame_index);
            if (buffer_state->bound_buffer_handles[frame_index] == buffer_handle)
            {
                return true;
            }

            instance_payload_data.resize(stride);
            std::fill(instance_payload_data.begin(), instance_payload_data.end(), std::uint8_t{ 0 });

            if (range.has_model_field && range.model_offset_in_range >= range.data_offset_in_range)
            {
                const std::size_t model_offset_in_data = range.model_offset_in_range - range.data_offset_in_range;
                if (model_offset_in_data + sizeof(glm::mat4) <= instance_payload_data.size())
                {
                    const glm::mat4 identity = glm::identity<glm::mat4>();
                    std::memcpy(
                        instance_payload_data.data() + model_offset_in_data,
                        &identity,
                        sizeof(glm::mat4));
                }
            }

            if (void* mapped = buffer_state->mapped_frame_data[frame_index])
            {
                std::memcpy(mapped, instance_payload_data.data(), stride);
            }
            else
            {
                buffer_state->buffer->upload(instance_payload_data.data(), stride, 0, frame_index);
            }

            material.update_buffer(range.instancing_storage_buffer_name, *buffer_state->buffer);
            buffer_state->bound_buffer_handles[frame_index] = buffer_handle;
            return true;
        };

        for (const DrawPlan& plan : plans)
        {
            if (!plan.group || !plan.group->material || !plan.group->mesh) continue;

            MaterialRenderInfo* render_info = get_material_render_info(plan.group->material);
            if (!ensure_instancing_descriptor_bound(*plan.group->material, render_info))
            {
                continue;
            }

            auto* gpu_mesh = get_or_create_gpu_mesh(plan.group->mesh);
            if (!gpu_mesh) continue;

            auto* vertex_buffer_rhi = static_cast<rhi::Buffer*>(gpu_mesh->vertex_buffer.rhi_handle());
            auto* index_buffer_rhi = static_cast<rhi::Buffer*>(gpu_mesh->index_buffer.rhi_handle());

            if (!vertex_buffer_rhi || !index_buffer_rhi) continue;

            plan.group->material->bind();

            cmd->bind_vertex_buffer(vertex_buffer_rhi);
            cmd->bind_index_buffer(index_buffer_rhi);

            bool draw_instanced = plan.use_instancing;
            if (draw_instanced)
            {
                draw_instanced = upload_instance_payload_for_plan(*plan.group, render_info);
            }

            if (draw_instanced)
            {
                push_ranges_for_draw(
                    *plan.group->material,
                    render_info,
                    nullptr,
                    true);

                cmd->draw_indexed(
                    gpu_mesh->index_count,
                    static_cast<std::uint32_t>(plan.group->active_instance_count),
                    0,
                    0,
                    0);
            }
            else
            {
                for (std::size_t instance_index = 0; instance_index < plan.group->active_instance_count; ++instance_index)
                {
                    const glm::mat4& model_matrix = plan.group->world_matrices[instance_index];

                    push_ranges_for_draw(
                        *plan.group->material,
                        render_info,
                        &model_matrix,
                        false);

                    cmd->draw_indexed(gpu_mesh->index_count);
                }
            }
        }

        const std::uint32_t image_idx = swapchain_->current_image_index();

        rhi::RenderContext::set_current_command_buffer(nullptr);
        if (!swapchain_->end_render_pass(image_idx))
        {
            reset_frame_draw_state();
            return;
        }

        if (!swapchain_->end_frame())
        {
            Log::error("Swapchain end_frame failed");
        }

        frame_render_active = false;

        reset_frame_draw_state();
    }


    void RenderingSystem::Render::execute(MeshRenderer& mr, const Transform& t)
    {
        if (!frame_render_active) return;

        DrawGroup* draw_group = static_cast<DrawGroup*>(mr.cached_draw_group_);
        Mesh* mesh = nullptr;
        Material* material = nullptr;

        if (draw_group)
        {
            mesh = draw_group->mesh;
            material = draw_group->material;

            if (!mesh || !material) return;

            if (draw_group->active_instance_count == 0)
            {
                draw_group_order.push_back(MeshMaterialKey{
                    .mesh = mesh,
                    .material = material
                });
            }
        }
        else
        {
            mesh = mr.mesh_;
            if (!mesh && !mr.mesh_name_.empty()) mesh = (mr.mesh_ = Mesh::try_get(mr.mesh_name_));
            if (!mesh) return;

            material = mr.material_;
            if (!material && !mr.material_name_.empty()) material = (mr.material_ = Material::try_get(mr.material_name_));

            if (!material)
            {
                static Material* default_material = nullptr;
                if (!default_material) default_material = gfx::MaterialLoader::instance().try_get_material("default");

                material = default_material;
                if (!material) return;

                mr.material_ = material;
            }

            const MeshMaterialKey key{
                .mesh = mesh,
                .material = material
            };

            auto [it, inserted] = draw_groups.try_emplace(
                key,
                DrawGroup{
                    .mesh = mesh,
                    .material = material
                });

            draw_group = &it->second;
            mr.cached_group_mesh_ = mesh;
            mr.cached_group_material_ = material;
            mr.cached_draw_group_ = draw_group;

            if (inserted || draw_group->active_instance_count == 0) draw_group_order.push_back(key);
        }

        const bool perform_cpu_cull = material->cpu_cull_enabled() && frustum_state.valid;

        const auto append_world_matrix = [draw_group](const glm::mat4& matrix)
        {
            const std::size_t write_index = draw_group->active_instance_count;

            if (write_index < draw_group->world_matrices.size())
            {
                draw_group->world_matrices[write_index] = matrix;
            }
            else
            {
                draw_group->world_matrices.emplace_back(matrix);
            }

            draw_group->active_instance_count = write_index + 1;
        };

        if (!perform_cpu_cull)
        {
            append_world_matrix(t.dirty_ ? t.world_matrix() : t.world_matrix_);
            return;
        }

        const glm::mat4 model_matrix = t.dirty_ ? t.world_matrix() : t.world_matrix_;

        const glm::vec3 world_center = glm::vec3(model_matrix * glm::vec4{ mesh->bounds.center, 1.0f });

        const float scale_x = length(glm::vec3{ model_matrix[0] });
        const float scale_y = length(glm::vec3{ model_matrix[1] });
        const float scale_z = length(glm::vec3{ model_matrix[2] });

        const float world_radius = mesh->bounds.radius * std::max({ scale_x, scale_y, scale_z });
        if (!frustum_state.sphere_visible(world_center, world_radius)) return;

        append_world_matrix(model_matrix);
    }

    void RenderingSystem::CameraUboUpdate::execute(const Camera& cam, const Transform& transform)
    {
        auto* camera_ubo = gfx::MaterialLoader::instance().camera_ubo();
        if (!camera_ubo)
        {
            frustum_state.valid = false;
            return;
        }

        const float aspect_ratio = rhi::RenderContext::window()->aspect_ratio();

        const gfx::CameraUBO ubo_data{
            .view = transform.view_matrix(),
            .proj = cam.projection_matrix(aspect_ratio)
        };

        frustum_state.set_from_view_projection(ubo_data.proj * ubo_data.view);

        camera_ubo->upload(&ubo_data, sizeof(gfx::CameraUBO), 0);
    }

    void RenderingSystem::EngineDestroy::execute()
    {
        if (device_) device_->wait_idle();

        gfx::MaterialLoader::instance().shutdown();
        gfx::SamplerLoader::instance().shutdown();
        gfx::TextureLoader::instance().shutdown();

        gpu_meshes_.clear();
        reset_frame_draw_state();
        draw_groups.clear();
        reset_render_caches();

        resource_cache_.reset();

        if (descriptor_pool_)
        {
            descriptor_pool_->destroy();
            descriptor_pool_.reset();
        }

        if (swapchain_)
        {
            swapchain_->destroy();
            swapchain_.reset();
        }

        if (device_)
        {
            device_->destroy();
            device_.reset();
        }

        if (instance_)
        {
            instance_->destroy();
            instance_.reset();
        }
    }


    bool RenderingSystem::init_graphics()
    {
        platform::Window* window = rhi::RenderContext::window();
        if (!window) return false;

        const auto cleanup_graphics_state = [window](const bool wait_for_device)
        {
            if (wait_for_device && device_) device_->wait_idle();

            gpu_meshes_.clear();
            reset_frame_draw_state();
            draw_groups.clear();
            reset_render_caches();
            resource_cache_.reset();

            if (descriptor_pool_)
            {
                descriptor_pool_->destroy();
                descriptor_pool_.reset();
            }

            if (swapchain_)
            {
                swapchain_->destroy();
                swapchain_.reset();
            }

            if (device_)
            {
                device_->destroy();
                device_.reset();
            }

            if (instance_)
            {
                instance_->destroy();
                instance_.reset();
            }

            window->destroy();
        };

        bool found = false;

        for (const auto& api : rhi::graphics_apis_by_priority)
        {
            if (api != rhi::graphics_apis_by_priority[0])
                cleanup_graphics_state(true);

            window->create(api);

            instance_.reset(create_instance(
                api, {
                    .app_name = "Boza Application",
                    .engine_name = "Boza",
                    .app_version = { 0, 0, 1 },
                    .engine_version = { BOZA_VERSION_MAJOR, BOZA_VERSION_MINOR, BOZA_VERSION_PATCH },
                    .window = window
                }));

            if (!instance_) continue;

            device_.reset(create_device(
                api, {
                    .instance = instance_.get(),
                    .window = window
                }));

            if (!device_) continue;

            swapchain_.reset(create_swapchain(
                api, {
                    .device = device_.get(),
                    .window = window,
                    .preferred_present_mode = app::GameSettings::gameplay.vsync
                        ? rhi::PresentMode::Fifo
                        : rhi::PresentMode::Mailbox,
                    .preferred_image_count = 3,
                    .max_frames_in_flight = 2,
                    .enable_depth = true,
                    .clear_color = { 0.1f, 0.1f, 0.15f, 1.0f },
                    .clear_depth = 1.0f,
                    .clear_stencil = 0
                }));

            if (!swapchain_) continue;

            descriptor_pool_.reset(create_descriptor_pool(
                api, {
                    .device = device_.get(),
                    .max_sets = 300,
                    .pool_sizes = {
                        { rhi::DescriptorType::UniformBuffer, 300 },
                        { rhi::DescriptorType::CombinedImageSampler, 300 },
                        { rhi::DescriptorType::StorageBuffer, 300 },
                        { rhi::DescriptorType::StorageImage, 100 }
                    }
                }));

            if (!descriptor_pool_) continue;

            resource_cache_ = std::make_unique<rhi::ResourceCache>();

            rhi::RenderContext::initialize(
                device_.get(),
                swapchain_.get(),
                resource_cache_.get(),
                descriptor_pool_.get(),
                api);

            found = true;
            break;
        }

        if (!found)
        {
            cleanup_graphics_state(false);
            return false;
        }

        window->show();
        return true;
    }

    void RenderingSystem::setup_resources()
    {
        gfx::TextureLoader::instance().initialize();

        gfx::SamplerLoader::instance().initialize();
        gfx::SamplerLoader::instance().load_and_create_samplers();

        gfx::MaterialLoader::instance().initialize(
            device_.get(),
            swapchain_.get(),
            descriptor_pool_.get(),
            resource_cache_.get(),
            rhi::RenderContext::api());

        gfx::MaterialLoader::instance().load_all_material_definitions();
        gfx::MaterialLoader::instance().create_game_load_materials();
    }

    void RenderingSystem::wait_idle() { if (device_) device_->wait_idle(); }


    GpuMesh* RenderingSystem::get_or_create_gpu_mesh(Mesh* mesh)
    {
        if (!mesh)
        {
            Log::error("get_or_create_gpu_mesh called with null mesh pointer");
            return nullptr;
        }

        const std::size_t vertex_buffer_size = mesh->vertices.size() * sizeof(Vertex);
        const std::size_t index_buffer_size = mesh->indices.size() * sizeof(std::uint32_t);

        if (vertex_buffer_size == 0 || index_buffer_size == 0)
        {
            Log::error("Mesh has empty vertices or indices");
            return nullptr;
        }

        if (const auto hit = gpu_meshes_.find(mesh); hit != gpu_meshes_.end())
            return &hit->second;

        auto [it, inserted] = gpu_meshes_.try_emplace(
            mesh,
            GpuMesh{
                .vertex_buffer = {
                    vertex_buffer_size,
                    BufferUsage::Vertex,
                    ResourceAccessMode::Static
                },
                .index_buffer = {
                    index_buffer_size,
                    BufferUsage::Index,
                    ResourceAccessMode::Static
                },
                .index_count = static_cast<std::uint32_t>(mesh->indices.size())
            }
        );

        if (inserted)
        {
            it->second.vertex_buffer.upload(mesh->vertices.data(), vertex_buffer_size, 0);
            it->second.index_buffer.upload(mesh->indices.data(), index_buffer_size, 0);
        }

        return &it->second;
    }
}
