module boza.gfx;

import :compute_dispatcher;
import :texture;
import :buffer;

import boza.rhi;
import boza.rhi.render_context;

import boza.core;

namespace boza
{
    using RhiBufferHandle = std::unique_ptr<rhi::Buffer, void(*)(rhi::Buffer*)>;
    static void destroy_rhi_buffer(rhi::Buffer* buffer) { delete buffer; }

    struct ComputeDispatcher::Impl
    {
        rhi::ComputePipeline* pipeline{ nullptr };
        rhi::PipelineLayout*  pipeline_layout{ nullptr };
        rhi::ShaderModule*    shader{ nullptr };
        rhi::DescriptorPool*  descriptor_pool{ nullptr };
        rhi::CommandPool*     pending_command_pool{ nullptr };
        rhi::CommandBuffer*   pending_command_buffer{ nullptr };
        std::unique_ptr<rhi::Fence> pending_fence{};

        rhi::DescriptorReflection reflection{};

        std::vector<rhi::DescriptorSet*>       descriptor_sets;
        std::vector<std::uint8_t>              push_constant_staging;
        flat_map<std::uint32_t, std::vector<std::uint8_t>> uniform_buffer_staging;
        flat_map<std::uint32_t, RhiBufferHandle>           uniform_buffers;

        flat_map<std::uint32_t, bool> dirty_sets;

        std::uint32_t push_constant_size{ 0 };
        bool          has_push_constants{ false };
        ComputeDispatchStatus dispatch_status{ ComputeDispatchStatus::Idle };
    };

    struct ComputeDispatchGroup::Impl
    {
        struct Step
        {
            ComputeDispatcher* dispatcher{ nullptr };
            glm::uvec3         dimensions{ 1, 1, 1 };
            bool               explicit_groups{ false };
            std::vector<std::uint32_t> wait_for{};
        };

        std::vector<Step> steps{};
        std::vector<std::uint64_t> recorded_generations{};

        rhi::CommandPool*   command_pool{ nullptr };
        rhi::CommandBuffer* command_buffer{ nullptr };
        std::unique_ptr<rhi::Fence> fence{};

        bool recorded{ false };
        bool dirty{ true };

        ComputeDispatchStatus dispatch_status{ ComputeDispatchStatus::Idle };
    };


    ComputeDispatcher::ComputeDispatcher(const std::string& shader_name) : impl_{ std::make_unique<Impl>() }
    {
        work_group_size_ = glm::uvec3{ 1, 1, 1 };

        if (!rhi::RenderContext::initialized() || !rhi::RenderContext::device())
        {
            Log::error("Render context not initialized. Cannot create compute dispatcher.");
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        auto  api             = rhi::RenderContext::api();
        auto* device          = rhi::RenderContext::device();
        auto* resource_cache  = rhi::RenderContext::resource_cache();
        auto* descriptor_pool = rhi::RenderContext::descriptor_pool();

        if (!resource_cache)
        {
            Log::error("ResourceCache not available in graphics context. Cannot create compute dispatcher.");
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        if (!descriptor_pool)
        {
            Log::error("DescriptorPool not available in graphics context. Cannot create compute dispatcher.");
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        const rhi::ShaderModuleDesc shader_desc
        {
            .device = device,
            .filename = shader_name + ".comp",
            .stage = rhi::ShaderStage::Compute
        };

        const auto shader_shared = resource_cache->get_or_create_shader(
            shader_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return create_shader_module(api, desc); });

        if (!shader_shared)
        {
            Log::error("Failed to load compute shader: {}", shader_name);
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        rhi::ShaderModule*    shader          = shader_shared.get();
        rhi::ComputePipeline* pipeline        = nullptr;
        rhi::PipelineLayout*  pipeline_layout = nullptr;
        std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;

        const auto cached = resource_cache->get_cached_compute_pipeline(shader_name);
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
            rhi::PipelineBuilder builder{ api, device, { shader } };

            if (!builder.build_descriptor_set_layouts())
            {
                Log::error("Failed to build descriptor set layouts for compute shader: {}", shader_name);
                failed_.store(true, std::memory_order_relaxed);
                return;
            }

            auto pipeline_layout_owner = builder.build_pipeline_layout();
            if (!pipeline_layout_owner)
            {
                Log::error("Failed to create pipeline layout for compute shader: {}", shader_name);
                failed_.store(true, std::memory_order_relaxed);
                return;
            }

            auto pipeline_owner = builder.build_compute_pipeline(pipeline_layout_owner.get());
            if (!pipeline_owner)
            {
                Log::error("Failed to create compute pipeline for shader: {}", shader_name);
                failed_.store(true, std::memory_order_relaxed);
                return;
            }

            auto built_descriptor_set_layouts = builder.take_descriptor_set_layouts();
            rhi::ResourceCache::CachedComputePipeline cached_pipeline;
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

            const auto stored_cached = resource_cache->cache_compute_pipeline(shader_name, std::move(cached_pipeline));

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

        std::vector<rhi::DescriptorSet*> descriptor_sets;
        descriptor_sets.reserve(descriptor_set_layouts.size());

        for (auto* layout : descriptor_set_layouts)
        {
            auto* desc_set = descriptor_pool->allocate_descriptor_set(layout);
            if (!desc_set)
            {
                Log::error("Failed to allocate descriptor set for compute dispatcher");

                if (!descriptor_sets.empty()) descriptor_pool->free_descriptor_sets(descriptor_sets);
                failed_.store(true, std::memory_order_relaxed);
                return;
            }
            descriptor_sets.push_back(desc_set);
        }

        impl_->pipeline               = pipeline;
        impl_->pipeline_layout        = pipeline_layout;
        impl_->shader                 = shader;
        impl_->descriptor_sets        = std::move(descriptor_sets);
        impl_->descriptor_pool        = descriptor_pool;

        impl_->reflection.build_from_shaders({ &shader, 1 });

        work_group_size_ = shader->meta_data().work_group_size;

        const auto& metadata = shader->meta_data();
        if (!metadata.push_constants.empty())
        {
            impl_->has_push_constants = true;
            for (const auto& [name, pc] : metadata.push_constants)
            {
                const std::uint32_t end = pc.offset + pc.size;
                impl_->push_constant_size = std::max(impl_->push_constant_size, end);
            }
        }

        impl_->push_constant_staging.resize(impl_->push_constant_size);

        // Log::trace("ComputeDispatcher created for shader: {}", shader_name);
    }

    ComputeDispatcher::~ComputeDispatcher()
    {
        wait();

        if (impl_->descriptor_pool && !impl_->descriptor_sets.empty())
        {
            impl_->descriptor_pool->free_descriptor_sets(impl_->descriptor_sets);
        }
    }

    // TODO: fix duplication; logic is very similar and can be shortened with a helper function
    ComputeDispatcher& ComputeDispatcher::set(const std::string& name, const Texture& texture)
    {
        std::scoped_lock lock{ mutex_ };
        if (failed_.load(std::memory_order_relaxed)) return *this;
        if (!wait_for_pending_dispatch_locked()) return *this;

        const auto binding_info = impl_->reflection.lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Compute texture '{}' not found in shader reflection", name);
            return *this;
        }

        const auto& info = binding_info.value();
        if (info.set >= impl_->descriptor_sets.size())
        {
            Log::error("Invalid descriptor set index {} for property '{}'", info.set, name);
            return *this;
        }

        auto* desc_set = impl_->descriptor_sets[info.set];
        if (!desc_set)
        {
            Log::error("Descriptor set {} is null", info.set);
            return *this;
        }

        if (info.descriptor_type != rhi::DescriptorType::StorageImage)
        {
            Log::warn("Compute property '{}' is not a storage image", name);
            return *this;
        }

        auto* texture_handle = static_cast<rhi::Texture*>(texture.rhi_handle());
        if (!texture_handle)
        {
            Log::error("Compute texture '{}' has an invalid RHI texture handle", name);
            return *this;
        }

        rhi::DescriptorWrite write
        {
            .binding = info.binding,
            .array_element = 0,
            .type = rhi::DescriptorType::StorageImage,
            .info = rhi::StorageImage{ .texture = texture_handle }
        };

        desc_set->update({ &write, 1 });
        mark_set_dirty(info.set);
        touch_generation();
        return *this;
    }

    ComputeDispatcher& ComputeDispatcher::set(const std::string& name, const Buffer& buffer)
    {
        std::scoped_lock lock{ mutex_ };
        if (failed_.load(std::memory_order_relaxed)) return *this;
        if (!wait_for_pending_dispatch_locked()) return *this;

        const auto binding_info = impl_->reflection.lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Compute buffer '{}' not found in shader reflection", name);
            return *this;
        }

        const auto& info = binding_info.value();
        if (info.descriptor_type != rhi::DescriptorType::StorageBuffer)
        {
            Log::warn("Compute property '{}' is not a storage buffer", name);
            return *this;
        }

        if (info.set >= impl_->descriptor_sets.size())
        {
            Log::error("Invalid descriptor set index {} for property '{}'", info.set, name);
            return *this;
        }

        auto* desc_set = impl_->descriptor_sets[info.set];
        if (!desc_set)
        {
            Log::error("Descriptor set {} is null", info.set);
            return *this;
        }

        auto* buffer_handle = static_cast<rhi::Buffer*>(buffer.rhi_handle());
        if (!buffer_handle)
        {
            Log::error("Compute buffer '{}' has an invalid RHI buffer handle", name);
            return *this;
        }

        rhi::DescriptorWrite write
        {
            .binding = info.binding,
            .array_element = 0,
            .type = rhi::DescriptorType::StorageBuffer,
            .info = rhi::StorageBuffer{
                .buffer = buffer_handle,
                .offset = 0,
                .range = static_cast<std::uint32_t>(buffer.size())
            }
        };

        desc_set->update({ &write, 1 });
        mark_set_dirty(info.set);
        touch_generation();
        return *this;
    }

    ComputeDispatcher& ComputeDispatcher::dispatch(
        const std::uint32_t width,
        const std::uint32_t height,
        const std::uint32_t depth)
    {
        return dispatch(glm::uvec3{ width, height, depth });
    }

    ComputeDispatcher& ComputeDispatcher::dispatch(const glm::uvec2& size)
    {
        return dispatch(glm::uvec3{ size, 1u });
    }

    ComputeDispatcher& ComputeDispatcher::dispatch(const glm::uvec3& size)
    {
        std::scoped_lock lock{ mutex_ };
        if (failed_.load(std::memory_order_relaxed)) return *this;
        if (!wait_for_pending_dispatch_locked()) return *this;

        const auto groups = resolve_group_counts(size, false);
        if (!groups.has_value())
        {
            failed_.store(true, std::memory_order_relaxed);
            return *this;
        }

        try { dispatch_impl(groups->x, groups->y, groups->z); }
        catch (...)
        {
            failed_.store(true, std::memory_order_relaxed);
            Log::error("Compute dispatch failed with exception");
        }

        return *this;
    }

    ComputeDispatcher& ComputeDispatcher::dispatch_groups(
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint32_t z)
    {
        return dispatch_groups(glm::uvec3{ x, y, z });
    }

    ComputeDispatcher& ComputeDispatcher::dispatch_groups(const glm::uvec2& groups)
    {
        return dispatch_groups(glm::uvec3{ groups, 1u });
    }

    ComputeDispatcher& ComputeDispatcher::dispatch_groups(const glm::uvec3& groups)
    {
        std::scoped_lock lock{ mutex_ };
        if (failed_.load(std::memory_order_relaxed)) return *this;
        if (!wait_for_pending_dispatch_locked()) return *this;

        const auto resolved_groups = resolve_group_counts(groups, true);
        if (!resolved_groups.has_value())
        {
            failed_.store(true, std::memory_order_relaxed);
            return *this;
        }

        try { dispatch_impl(resolved_groups->x, resolved_groups->y, resolved_groups->z); }
        catch (...)
        {
            failed_.store(true, std::memory_order_relaxed);
            Log::error("Compute dispatch failed with exception");
        }

        return *this;
    }

    ComputeDispatcher& ComputeDispatcher::wait()
    {
        std::scoped_lock lock{ mutex_ };
        (void)wait_for_pending_dispatch_locked();
        return *this;
    }

    ComputeDispatchStatus ComputeDispatcher::status() const
    {
        std::scoped_lock lock{ mutex_ };

        if (failed_.load(std::memory_order_relaxed)) return ComputeDispatchStatus::Failed;

        poll_pending_dispatch_locked();

        if (failed_.load(std::memory_order_relaxed)) return ComputeDispatchStatus::Failed;

        return impl_->dispatch_status;
    }

    bool ComputeDispatcher::wait_for_pending_dispatch_locked() const
    {
        if (!impl_->pending_command_buffer) return true;

        bool success = true;
        if (impl_->pending_fence && !impl_->pending_fence->wait())
        {
            Log::error("Compute dispatch fence wait failed");
            failed_.store(true, std::memory_order_relaxed);
            impl_->dispatch_status = ComputeDispatchStatus::Failed;
            success = false;
        }

        if (impl_->pending_command_pool && impl_->pending_command_buffer)
        {
            impl_->pending_command_pool->free_command_buffer(impl_->pending_command_buffer);
        }

        impl_->pending_command_buffer = nullptr;
        impl_->pending_command_pool = nullptr;
        impl_->pending_fence.reset();

        if (success) impl_->dispatch_status = ComputeDispatchStatus::Finished;

        return success;
    }

    void ComputeDispatcher::poll_pending_dispatch_locked() const
    {
        if (!impl_->pending_command_buffer) return;

        if (impl_->pending_fence && !impl_->pending_fence->is_signaled())
        {
            impl_->dispatch_status = ComputeDispatchStatus::Running;
            return;
        }

        if (impl_->pending_command_pool && impl_->pending_command_buffer)
        {
            impl_->pending_command_pool->free_command_buffer(impl_->pending_command_buffer);
        }

        impl_->pending_command_buffer = nullptr;
        impl_->pending_command_pool = nullptr;
        impl_->pending_fence.reset();
        impl_->dispatch_status = ComputeDispatchStatus::Finished;
    }

    std::optional<glm::uvec3> ComputeDispatcher::resolve_group_counts(
        const glm::uvec3& dimensions,
        const bool explicit_groups) const
    {
        if (dimensions.x == 0 || dimensions.y == 0 || dimensions.z == 0)
        {
            if (explicit_groups)
            {
                Log::error(
                    "Dispatch group counts must be greater than zero ({}x{}x{})",
                    dimensions.x,
                    dimensions.y,
                    dimensions.z);
            }
            else
            {
                Log::error(
                    "Dispatch dimensions must be greater than zero ({}x{}x{})",
                    dimensions.x,
                    dimensions.y,
                    dimensions.z);
            }

            return std::nullopt;
        }

        if (explicit_groups) return dimensions;

        if (work_group_size_.x == 0 || work_group_size_.y == 0 || work_group_size_.z == 0)
        {
            Log::error(
                "Invalid shader workgroup size ({}x{}x{})",
                work_group_size_.x,
                work_group_size_.y,
                work_group_size_.z);
            return std::nullopt;
        }

        return glm::uvec3{
            (dimensions.x + work_group_size_.x - 1u) / work_group_size_.x,
            (dimensions.y + work_group_size_.y - 1u) / work_group_size_.y,
            (dimensions.z + work_group_size_.z - 1u) / work_group_size_.z
        };
    }

    void ComputeDispatcher::dispatch_impl(const std::uint32_t x, const std::uint32_t y, const std::uint32_t z)
    {
        if (!impl_->pipeline)
        {
            Log::error("Cannot dispatch compute: pipeline is null");
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        const auto* device = rhi::RenderContext::device();
        if (!device)
        {
            Log::error("Cannot dispatch compute: device is null");
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        auto* cmd_pool = device->command_pool(device->queue_family_indices().compute_family);
        if (!cmd_pool)
        {
            Log::error("Cannot dispatch compute: compute command pool is null");
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        auto* cmd = cmd_pool->allocate_command_buffer(true);
        if (!cmd)
        {
            Log::error("Cannot dispatch compute: failed to allocate command buffer");
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        if (!cmd->begin(rhi::CommandBufferUsage::OneTimeSubmit))
        {
            Log::error("Cannot dispatch compute: failed to begin command buffer");
            cmd_pool->free_command_buffer(cmd);
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        cmd->bind_compute_pipeline(impl_->pipeline);

        if (!impl_->descriptor_sets.empty())
        {
            cmd->bind_descriptor_sets(impl_->pipeline->get_layout(), impl_->descriptor_sets, 0);
        }

        if (impl_->has_push_constants && impl_->push_constant_size > 0)
        {
            cmd->push_constants(
                impl_->pipeline_layout,
                rhi::ShaderStage::Compute,
                0,
                impl_->push_constant_size,
                impl_->push_constant_staging.data()
            );
        }

        cmd->dispatch(x, y, z);

        if (!cmd->end())
        {
            Log::error("Cannot dispatch compute: failed to end command buffer");
            cmd_pool->free_command_buffer(cmd);
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        auto fence = create_fence(rhi::RenderContext::api(), {
            .device = const_cast<rhi::Device*>(device),
            .signaled = false
        });

        if (!fence)
        {
            Log::error("Cannot dispatch compute: failed to create fence");
            cmd_pool->free_command_buffer(cmd);
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        auto* queue = device->queue(device->queue_family_indices().compute_family);
        if (!queue)
        {
            Log::error("Cannot dispatch compute: compute queue is null");
            cmd_pool->free_command_buffer(cmd);
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        if (!queue->submit({
            .command_buffers = { cmd },
            .signal_fence = fence.get()
        }))
        {
            Log::error("Cannot dispatch compute: failed to submit command buffer");
            cmd_pool->free_command_buffer(cmd);
            failed_.store(true, std::memory_order_relaxed);
            return;
        }

        impl_->pending_command_pool = cmd_pool;
        impl_->pending_command_buffer = cmd;
        impl_->pending_fence = std::move(fence);
        impl_->dispatch_status = ComputeDispatchStatus::Running;

        impl_->dirty_sets.clear();
    }

    void ComputeDispatcher::touch_generation() const
    {
        generation_.fetch_add(1, std::memory_order_relaxed);
    }

    void ComputeDispatcher::mark_set_dirty(const std::uint32_t set) const { impl_->dirty_sets[set] = true; }

    void ComputeDispatcher::update_property_impl(
        const std::string& name,
        const void*        data,
        const std::size_t  size,
        [[maybe_unused]]
        const ShaderDataType type) const
    {
        std::scoped_lock lock{ mutex_ };
        if (failed_.load(std::memory_order_relaxed)) return;
        if (!wait_for_pending_dispatch_locked()) return;

        const auto binding_info = impl_->reflection.lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Compute property '{}' not found in shader reflection", name);
            return;
        }

        const auto& info = binding_info.value();

        #ifdef BOZA_DEBUG
        if (!rhi::validate_property_type(type, size, info.data_type, info.size))
        {
            const auto error_msg = rhi::format_type_mismatch_error(
                name,
                type,
                size,
                info.data_type,
                info.size
            );
            Log::warn("{}", error_msg);
        }
        #endif

        if (info.is_push_constant)
        {
            if (info.offset + size > impl_->push_constant_staging.size())
            {
                Log::error("Push constant '{}' offset {} + size {} exceeds staging buffer size {}",
                           name, info.offset, size, impl_->push_constant_staging.size());
                return;
            }

            std::memcpy(impl_->push_constant_staging.data() + info.offset, data, size);
            touch_generation();
            return;
        }

        if (info.descriptor_type != rhi::DescriptorType::UniformBuffer)
        {
            Log::warn("Compute property '{}' is not a uniform buffer member", name);
            return;
        }

        if (info.set >= impl_->descriptor_sets.size())
        {
            Log::error("Invalid descriptor set index {} for property '{}'", info.set, name);
            return;
        }

        auto* desc_set = impl_->descriptor_sets[info.set];
        if (!desc_set)
        {
            Log::error("Descriptor set {} is null", info.set);
            return;
        }

        const std::uint32_t binding_key = (info.set << 16) | info.binding;

        if (!impl_->uniform_buffers.contains(binding_key))
        {
            const auto parent_info = impl_->reflection.lookup(name.substr(0, name.find('.')));
            const std::size_t buffer_size = parent_info.has_value()
                                                ? parent_info->size
                                                : std::max<std::size_t>(info.offset + size, 256);

            auto buffer = create_buffer(
                rhi::RenderContext::api(), {
                    .device = rhi::RenderContext::device(),
                    .size = buffer_size,
                    .usage = BufferUsage::Uniform,
                    .memory_type = rhi::BufferMemoryType::HostVisible
                });

            if (!buffer)
            {
                Log::error("Failed to create compute uniform buffer for property '{}'", name);
                return;
            }

            auto* raw_buffer = buffer.get();
            impl_->uniform_buffers.try_emplace(
                binding_key,
                RhiBufferHandle{ buffer.release(), &destroy_rhi_buffer });
            impl_->uniform_buffer_staging[binding_key].resize(buffer_size, 0);

            const rhi::DescriptorWrite write{
                .binding = info.binding,
                .array_element = 0,
                .type = rhi::DescriptorType::UniformBuffer,
                .info = rhi::UniformBuffer{
                    .buffer = raw_buffer,
                    .offset = 0,
                    .range = static_cast<std::uint32_t>(buffer_size)
                }
            };

            std::array writes{ write };
            desc_set->update(writes);
        }

        auto& staging = impl_->uniform_buffer_staging.at(binding_key);
        if (info.offset + size > staging.size())
        {
            Log::error("Compute uniform property '{}' offset {} + size {} exceeds buffer size {}",
                       name, info.offset, size, staging.size());
            return;
        }

        std::memcpy(staging.data() + info.offset, data, size);

        auto* uniform_buffer = impl_->uniform_buffers.at(binding_key).get();
        if (!uniform_buffer)
        {
            Log::error("Compute uniform property '{}' has an invalid RHI buffer handle", name);
            return;
        }

        uniform_buffer->upload(staging.data(), staging.size(), 0);
        mark_set_dirty(info.set);
        touch_generation();
    }


    ComputeDispatchGroup::ComputeDispatchGroup() : impl_{ std::make_unique<Impl>() } {}

    ComputeDispatchGroup::~ComputeDispatchGroup()
    {
        std::scoped_lock lock{ mutex_ };

        (void)wait_for_pending_submit_locked();

        if (impl_->command_pool && impl_->command_buffer)
        {
            impl_->command_pool->free_command_buffer(impl_->command_buffer);
        }

        impl_->command_buffer = nullptr;
        impl_->command_pool = nullptr;
        impl_->fence.reset();
    }


    ComputeDispatchGroup& ComputeDispatchGroup::append_step(
        ComputeDispatcher& dispatcher,
        const glm::uvec3& dimensions,
        const bool        explicit_groups,
        const std::initializer_list<std::uint32_t> wait_for)
    {
        std::scoped_lock lock{ mutex_ };

        if (failed_.load(std::memory_order_relaxed)) return *this;

        std::vector<std::uint32_t> wait_indices;
        wait_indices.reserve(wait_for.size());
        for (const std::uint32_t index : wait_for)
            wait_indices.push_back(index);

        impl_->steps.push_back({
            .dispatcher = &dispatcher,
            .dimensions = dimensions,
            .explicit_groups = explicit_groups,
            .wait_for = std::move(wait_indices)
        });

        impl_->dirty = true;
        impl_->recorded = false;
        impl_->dispatch_status = ComputeDispatchStatus::Idle;
        return *this;
    }

    ComputeDispatchGroup& ComputeDispatchGroup::add(
        ComputeDispatcher& dispatcher,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::uint32_t depth,
        const std::initializer_list<std::uint32_t> wait_for)
    {
        return append_step(dispatcher, { width, height, depth }, false, wait_for);
    }

    ComputeDispatchGroup& ComputeDispatchGroup::add(
        ComputeDispatcher& dispatcher,
        const glm::uvec2& size,
        const std::initializer_list<std::uint32_t> wait_for)
    {
        return add(dispatcher, size.x, size.y, 1u, wait_for);
    }

    ComputeDispatchGroup& ComputeDispatchGroup::add(
        ComputeDispatcher& dispatcher,
        const glm::uvec3& size,
        const std::initializer_list<std::uint32_t> wait_for)
    {
        return add(dispatcher, size.x, size.y, size.z, wait_for);
    }

    ComputeDispatchGroup& ComputeDispatchGroup::add_groups(
        ComputeDispatcher& dispatcher,
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint32_t z,
        const std::initializer_list<std::uint32_t> wait_for)
    {
        return append_step(dispatcher, { x, y, z }, true, wait_for);
    }

    ComputeDispatchGroup& ComputeDispatchGroup::add_groups(
        ComputeDispatcher& dispatcher,
        const glm::uvec2&  groups,
        const std::initializer_list<std::uint32_t> wait_for)
    {
        return add_groups(dispatcher, groups.x, groups.y, 1u, wait_for);
    }

    ComputeDispatchGroup& ComputeDispatchGroup::add_groups(
        ComputeDispatcher& dispatcher,
        const glm::uvec3&  groups,
        const std::initializer_list<std::uint32_t> wait_for)
    {
        return add_groups(dispatcher, groups.x, groups.y, groups.z, wait_for);
    }


    ComputeDispatchGroup& ComputeDispatchGroup::clear()
    {
        std::scoped_lock lock{ mutex_ };

        if (failed_.load(std::memory_order_relaxed)) return *this;
        if (!wait_for_pending_submit_locked()) return *this;

        impl_->steps.clear();
        impl_->recorded_generations.clear();
        impl_->recorded = false;
        impl_->dirty = true;
        impl_->dispatch_status = ComputeDispatchStatus::Idle;
        return *this;
    }

    ComputeDispatchGroup& ComputeDispatchGroup::record()
    {
        std::scoped_lock lock{ mutex_ };

        if (failed_.load(std::memory_order_relaxed)) return *this;
        if (!wait_for_pending_submit_locked()) return *this;

        (void)record_locked();
        return *this;
    }

    ComputeDispatchGroup& ComputeDispatchGroup::submit()
    {
        std::scoped_lock lock{ mutex_ };

        if (failed_.load(std::memory_order_relaxed)) return *this;

        poll_pending_submit_locked();
        if (impl_->dispatch_status == ComputeDispatchStatus::Running)
        {
            Log::warn("Compute dispatch group is already running; skip submit");
            return *this;
        }

        if (!ensure_recorded_locked()) return *this;

        auto* device = rhi::RenderContext::device();
        if (!device)
        {
            Log::error("Cannot submit compute dispatch group: device is null");
            failed_.store(true, std::memory_order_relaxed);
            impl_->dispatch_status = ComputeDispatchStatus::Failed;
            return *this;
        }

        if (!impl_->fence)
        {
            impl_->fence = create_fence(rhi::RenderContext::api(), {
                .device = device,
                .signaled = false
            });

            if (!impl_->fence)
            {
                Log::error("Cannot submit compute dispatch group: failed to create fence");
                failed_.store(true, std::memory_order_relaxed);
                impl_->dispatch_status = ComputeDispatchStatus::Failed;
                return *this;
            }
        }
        else if (impl_->fence->is_signaled() && !impl_->fence->reset())
        {
            Log::error("Cannot submit compute dispatch group: failed to reset fence");
            failed_.store(true, std::memory_order_relaxed);
            impl_->dispatch_status = ComputeDispatchStatus::Failed;
            return *this;
        }

        auto* queue = device->queue(device->queue_family_indices().compute_family);
        if (!queue)
        {
            Log::error("Cannot submit compute dispatch group: compute queue is null");
            failed_.store(true, std::memory_order_relaxed);
            impl_->dispatch_status = ComputeDispatchStatus::Failed;
            return *this;
        }

        if (!queue->submit({
            .command_buffers = { impl_->command_buffer },
            .signal_fence = impl_->fence.get()
        }))
        {
            Log::error("Cannot submit compute dispatch group: queue submit failed");
            failed_.store(true, std::memory_order_relaxed);
            impl_->dispatch_status = ComputeDispatchStatus::Failed;
            return *this;
        }

        impl_->dispatch_status = ComputeDispatchStatus::Running;
        return *this;
    }

    ComputeDispatchGroup& ComputeDispatchGroup::wait()
    {
        std::scoped_lock lock{ mutex_ };
        (void)wait_for_pending_submit_locked();
        return *this;
    }

    ComputeDispatchGroup& ComputeDispatchGroup::run()
    {
        return record().submit().wait();
    }

    ComputeDispatchStatus ComputeDispatchGroup::status() const
    {
        std::scoped_lock lock{ mutex_ };

        if (failed_.load(std::memory_order_relaxed)) return ComputeDispatchStatus::Failed;

        poll_pending_submit_locked();

        if (failed_.load(std::memory_order_relaxed)) return ComputeDispatchStatus::Failed;
        return impl_->dispatch_status;
    }


    bool ComputeDispatchGroup::record_locked()
    {
        if (impl_->steps.empty())
        {
            Log::warn("Cannot record compute dispatch group: no steps added");
            return false;
        }

        const auto fail_record = [this](const std::string_view error_message) -> bool
        {
            Log::error("{}", error_message);
            failed_.store(true, std::memory_order_relaxed);
            impl_->dispatch_status = ComputeDispatchStatus::Failed;
            impl_->recorded = false;
            impl_->dirty = true;
            return false;
        };

        auto* device = rhi::RenderContext::device();
        if (!device) return fail_record("Cannot record compute dispatch group: device is null");

        auto* command_pool = device->command_pool(device->queue_family_indices().compute_family);
        if (!command_pool) return fail_record("Cannot record compute dispatch group: compute command pool is null");

        if (!impl_->command_buffer || impl_->command_pool != command_pool)
        {
            if (impl_->command_pool && impl_->command_buffer)
            {
                impl_->command_pool->free_command_buffer(impl_->command_buffer);
            }

            impl_->command_pool = command_pool;
            impl_->command_buffer = command_pool->allocate_command_buffer(true);

            if (!impl_->command_buffer)
                return fail_record("Cannot record compute dispatch group: failed to allocate command buffer");
        }

        if (!impl_->command_buffer->reset())
            return fail_record("Cannot record compute dispatch group: failed to reset command buffer");

        if (!impl_->command_buffer->begin())
            return fail_record("Cannot record compute dispatch group: failed to begin command buffer");

        impl_->recorded_generations.clear();
        impl_->recorded_generations.reserve(impl_->steps.size());

        bool has_recorded_step = false;

        for (std::size_t step_index = 0; step_index < impl_->steps.size(); ++step_index)
        {
            const Impl::Step& step = impl_->steps[step_index];

            if (!step.dispatcher)
            {
                (void)impl_->command_buffer->reset(true);
                return fail_record("Cannot record compute dispatch group: encountered null dispatcher step");
            }

            bool needs_barrier = false;
            for (const std::uint32_t dependency : step.wait_for)
            {
                if (dependency >= impl_->steps.size())
                {
                    Log::error(
                        "Compute dispatch group step {} references invalid dependency index {}",
                        step_index,
                        dependency);

                    (void)impl_->command_buffer->reset(true);
                    return fail_record("Cannot record compute dispatch group: invalid dependency index");
                }

                if (dependency >= step_index)
                {
                    Log::error(
                        "Compute dispatch group step {} dependency {} must reference an earlier stage",
                        step_index,
                        dependency);

                    (void)impl_->command_buffer->reset(true);
                    return fail_record("Cannot record compute dispatch group: dependency must reference earlier stage");
                }

                needs_barrier = true;
            }

            if (needs_barrier && has_recorded_step)
            {
                impl_->command_buffer->compute_memory_barrier();
            }

            auto* dispatcher = step.dispatcher;
            std::scoped_lock dispatcher_lock{ dispatcher->mutex_ };

            if (dispatcher->failed_.load(std::memory_order_relaxed))
            {
                (void)impl_->command_buffer->reset(true);
                return fail_record("Cannot record compute dispatch group: dispatcher is in failed state");
            }

            if (!dispatcher->wait_for_pending_dispatch_locked())
            {
                (void)impl_->command_buffer->reset(true);
                return fail_record("Cannot record compute dispatch group: dispatcher wait failed");
            }

            const auto groups = dispatcher->resolve_group_counts(step.dimensions, step.explicit_groups);
            if (!groups.has_value())
            {
                (void)impl_->command_buffer->reset(true);
                return fail_record("Cannot record compute dispatch group: invalid dispatch dimensions");
            }

            if (!dispatcher->impl_->pipeline)
            {
                Log::error("Cannot record compute dispatch group: dispatcher pipeline is null");
                dispatcher->failed_.store(true, std::memory_order_relaxed);

                (void)impl_->command_buffer->reset(true);
                return fail_record("Cannot record compute dispatch group: dispatcher pipeline is null");
            }

            impl_->command_buffer->bind_compute_pipeline(dispatcher->impl_->pipeline);

            if (!dispatcher->impl_->descriptor_sets.empty())
            {
                impl_->command_buffer->bind_descriptor_sets(
                    dispatcher->impl_->pipeline->get_layout(),
                    dispatcher->impl_->descriptor_sets,
                    0);
            }

            if (dispatcher->impl_->has_push_constants && dispatcher->impl_->push_constant_size > 0)
            {
                impl_->command_buffer->push_constants(
                    dispatcher->impl_->pipeline_layout,
                    rhi::ShaderStage::Compute,
                    0,
                    dispatcher->impl_->push_constant_size,
                    dispatcher->impl_->push_constant_staging.data());
            }

            impl_->command_buffer->dispatch(groups->x, groups->y, groups->z);

            impl_->recorded_generations.push_back(dispatcher->generation());
            has_recorded_step = true;
        }

        if (!impl_->command_buffer->end())
            return fail_record("Cannot record compute dispatch group: failed to end command buffer");

        impl_->recorded = true;
        impl_->dirty = false;
        impl_->dispatch_status = ComputeDispatchStatus::Idle;
        return true;
    }

    bool ComputeDispatchGroup::ensure_recorded_locked()
    {
        if (impl_->steps.empty())
        {
            Log::warn("Cannot submit compute dispatch group: no steps added");
            return false;
        }

        bool stale = impl_->dirty || !impl_->recorded;

        if (!stale)
        {
            if (impl_->recorded_generations.size() != impl_->steps.size()) stale = true;
            else
            {
                for (std::size_t i = 0; i < impl_->steps.size(); ++i)
                {
                    const auto* dispatcher = impl_->steps[i].dispatcher;
                    if (!dispatcher || dispatcher->generation() != impl_->recorded_generations[i])
                    {
                        stale = true;
                        break;
                    }
                }
            }
        }

        if (!stale) return true;
        return record_locked();
    }

    bool ComputeDispatchGroup::wait_for_pending_submit_locked() const
    {
        if (impl_->dispatch_status != ComputeDispatchStatus::Running) return true;

        if (!impl_->fence)
        {
            Log::error("Compute dispatch group has no fence while marked as running");
            failed_.store(true, std::memory_order_relaxed);
            impl_->dispatch_status = ComputeDispatchStatus::Failed;
            return false;
        }

        if (!impl_->fence->wait())
        {
            Log::error("Compute dispatch group fence wait failed");
            failed_.store(true, std::memory_order_relaxed);
            impl_->dispatch_status = ComputeDispatchStatus::Failed;
            return false;
        }

        impl_->dispatch_status = ComputeDispatchStatus::Finished;
        return true;
    }

    void ComputeDispatchGroup::poll_pending_submit_locked() const
    {
        if (impl_->dispatch_status != ComputeDispatchStatus::Running) return;

        if (!impl_->fence)
        {
            Log::error("Compute dispatch group has no fence while marked as running");
            failed_.store(true, std::memory_order_relaxed);
            impl_->dispatch_status = ComputeDispatchStatus::Failed;
            return;
        }

        if (!impl_->fence->is_signaled()) return;
        impl_->dispatch_status = ComputeDispatchStatus::Finished;
    }
}
