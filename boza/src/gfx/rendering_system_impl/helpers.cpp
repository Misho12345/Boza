module boza.gfx;

import :buffer;
import :material;
import :rendering_system;
import :rendering_system_common;

import boza.core;
import boza.rhi;
import boza.rhi.render_context;
import boza.gfx.material_loader;

namespace boza
{
    namespace
    {
        std::mutex       render_thread_mutex_{};
        std::thread::id  render_thread_{};
        std::atomic_bool render_thread_set_{ false };
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
        planes[4] = normalize_plane(m[3] + m[2]);
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


    void RenderingSystem::rebuild_pipeline_materials()
    {
        pipeline_materials_.clear();

        for (auto& [material_ptr, group] : render_cache_)
        {
            auto* pipeline = static_cast<rhi::GraphicsPipeline*>(MaterialAccess::pipeline(*material_ptr));
            if (!pipeline) continue;

            auto& materials_vec = pipeline_materials_[pipeline];

            if (std::ranges::find(materials_vec, material_ptr) == materials_vec.end())
                materials_vec.push_back(material_ptr);
        }
    }


    void RenderingSystem::remove_from_render_cache(MeshRenderer& mr, GameObject go)
    {
        if (!mr.in_render_cache_) return;

        Material* mat = mr.cached_material_;
        Mesh* mesh = mr.cached_mesh_;

        if (mat && mesh)
        {
            if (auto mat_it = render_cache_.find(mat); mat_it != render_cache_.end())
            {
                auto& mat_group = mat_it->second;
                if (auto mesh_it = mat_group.mesh_buckets.find(mesh); mesh_it != mat_group.mesh_buckets.end())
                {
                    auto& bucket = mesh_it->second;
                    auto elem_it = std::ranges::find_if(bucket.elements,
                        [go](const RenderElement& el) { return el.entity == go; });

                    if (elem_it != bucket.elements.end())
                    {
                        if (elem_it != bucket.elements.end() - 1)
                            *elem_it = std::move(bucket.elements.back());
                        bucket.elements.pop_back();
                    }

                    if (bucket.elements.empty())
                        mat_group.mesh_buckets.erase(mesh_it);
                }

                if (mat_group.mesh_buckets.empty())
                    render_cache_.erase(mat_it);
            }
        }

        mr.in_render_cache_ = false;
        pipeline_materials_dirty_ = true;
    }

    void RenderingSystem::remove_from_unresolved(MeshRenderer& mr, GameObject go)
    {
        if (!mr.in_unresolved_cache_) return;

        auto it = std::ranges::find(unresolved_, go);
        if (it != unresolved_.end())
        {
            if (it != unresolved_.end() - 1)
                *it = std::move(unresolved_.back());
            unresolved_.pop_back();
        }

        mr.in_unresolved_cache_ = false;
    }

    void RenderingSystem::insert_into_render_cache(MeshRenderer& mr, GameObject go, Mesh* mesh, Material* material)
    {
        auto& mat_group = render_cache_[material];
        auto& bucket = mat_group.mesh_buckets[mesh];

        bucket.elements.emplace_back(go, false);

        mr.in_render_cache_ = true;
        mr.cached_mesh_ = mesh;
        mr.cached_material_ = material;
        mr.mesh_ = mesh;
        mr.material_ = material;
        pipeline_materials_dirty_ = true;
    }

    void RenderingSystem::insert_into_unresolved(MeshRenderer& mr, GameObject go)
    {
        unresolved_.push_back(go);
        mr.in_unresolved_cache_ = true;
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
        push_constant_scratch_.clear();
        instance_payload_scratch_.clear();
    }


    InstanceBufferState* get_instance_buffer(
        Material* material,
        const std::string& name)
    {
        if (!material) return nullptr;

        auto& per_binding = instance_buffers_[material];
        if (!per_binding.contains(name))
            per_binding.emplace(name, InstanceBufferState{});

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

        state.buffer = std::move(new_buffer);
        state.capacity_bytes = new_capacity;
        return true;
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

            for (const auto& member : range_info.members)
            {
                if (member.name == instancing_field_name_)
                {
                    use_instancing_member = &member;
                    continue;
                }
                data_members.push_back(&member);
            }

            runtime_range.has_use_instancing_field = use_instancing_member != nullptr;
            if (use_instancing_member)
                runtime_range.use_instancing_offset_in_range = use_instancing_member->offset;

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

        if (range_info.members.empty() || range_info.members.size() > 2)
        {
            Log::warn(
                "Material '{}' push constant range '{}' must contain only one data field T and optional uint use_instancing",
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

        if (!runtime_range.has_use_instancing_field && range_info.members.size() != 1)
        {
            Log::warn(
                "Material '{}' push constant range '{}' has unsupported fields; expected exactly one data field",
                material_name, range_name);
            valid_layout = false;
        }

        if (runtime_range.has_use_instancing_field)
        {
            if (range_info.members.size() != 2)
            {
                Log::warn(
                    "Material '{}' push constant range '{}' has unsupported fields; expected data field + uint use_instancing",
                    material_name, range_name);
                valid_layout = false;
            }

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
        const bool use_instancing)
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


    std::size_t gather_matrices_fast(
        const MeshBucket& bucket,
        std::vector<std::uint8_t>& payload)
    {
        std::size_t written = 0;

        for (const auto& elem : bucket.elements)
        {
            if (!elem.visible) continue;

            const auto* transform = std::as_const(elem.entity).try_get_component<Transform>();
            if (!transform) continue;

            const glm::mat4 model_matrix = transform->world_matrix();

            std::memcpy(
                payload.data() + written * sizeof(glm::mat4),
                &model_matrix,
                sizeof(glm::mat4));

            ++written;
        }

        return written;
    }

    std::size_t gather_instance_data(
        const MeshBucket& bucket,
        const std::span<const std::uint8_t>& push_staging,
        const std::size_t data_base_offset,
        const std::size_t base_copy_size,
        const bool can_write_model,
        const std::size_t model_offset_in_data,
        const PushConstantRangeRuntime& range,
        std::vector<std::uint8_t>& payload)
    {
        const bool base_copy_redundant =
            can_write_model &&
            model_offset_in_data == 0 &&
            base_copy_size <= sizeof(glm::mat4);

        std::size_t written = 0;

        for (const auto& elem : bucket.elements)
        {
            if (!elem.visible) continue;

            const auto* transform = std::as_const(elem.entity).try_get_component<Transform>();
            if (!transform) continue;

            std::uint8_t* dst =
                payload.data() +
                written * static_cast<std::size_t>(range.instancing_stride);

            if (base_copy_size > 0 && !base_copy_redundant)
                std::memcpy(dst, push_staging.data() + data_base_offset, base_copy_size);

            if (can_write_model)
            {
                const glm::mat4 model_matrix = transform->world_matrix();

                std::memcpy(
                    dst + model_offset_in_data,
                    &model_matrix,
                    sizeof(glm::mat4));
            }

            ++written;
        }

        return written;
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

    bool upload_instance_payload(
        Material* material,
        const MeshBucket& bucket,
        const PushConstantRangeRuntime& range)
    {
        if (!material) return false;

        if (!range.instancing_supported ||
            !range.has_data_member ||
            range.data_size == 0 ||
            range.instancing_stride == 0 ||
            range.instancing_stride < range.data_size)
            return false;

        const std::size_t instance_count = bucket.num_visible;
        if (instance_count == 0 ||
            instance_count > std::numeric_limits<std::uint32_t>::max()) return false;

        const std::size_t payload_size = instance_count * static_cast<std::size_t>(range.instancing_stride);
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

        const bool matrix_only_payload =
            can_write_model &&
            model_offset_in_data == 0 &&
            range.data_size == sizeof(glm::mat4) &&
            range.instancing_stride == sizeof(glm::mat4);

        std::size_t written = 0;

        if (matrix_only_payload)
        {
            written = gather_matrices_fast(bucket, instance_payload_scratch_);
        }
        else
        {
            written = gather_instance_data(
                bucket, push_staging, data_base_offset, base_copy_size,
                can_write_model, model_offset_in_data, range,
                instance_payload_scratch_);
        }

        if (written == 0) return false;

        const std::size_t actual_payload_size = written * static_cast<std::size_t>(range.instancing_stride);

        return upload_to_instance_buffer(material, range.instancing_ssbo_name, actual_payload_size);
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


    void RenderingSystem::on_mesh_destroyed(Mesh* mesh)
    {
        assert_render_thread();

        if (!mesh) return;

        destroyed_meshes_this_frame_.insert(mesh);

        for (auto& [mat_ptr, mat_group] : render_cache_)
        {
            auto mesh_it = mat_group.mesh_buckets.find(mesh);
            if (mesh_it == mat_group.mesh_buckets.end()) continue;

            for (auto& elem : mesh_it->second.elements)
            {
                if (elem.entity.valid())
                    elem.entity.add_component<tags::RenderCacheInvalidated>();
            }
        }

        valid_meshes_.erase(mesh);
        invalid_meshes_.insert(mesh);
    }

    void RenderingSystem::on_material_destroyed(Material* material)
    {
        assert_render_thread();

        if (!material) return;

        destroyed_materials_this_frame_.insert(material);

        auto mat_it = render_cache_.find(material);
        if (mat_it != render_cache_.end())
        {
            for (auto& [mesh_ptr, bucket] : mat_it->second.mesh_buckets)
            {
                for (auto& elem : bucket.elements)
                {
                    if (elem.entity.valid())
                        elem.entity.add_component<tags::RenderCacheInvalidated>();
                }
            }
        }

        valid_materials_.erase(material);
        invalid_materials_.insert(material);
    }
}
