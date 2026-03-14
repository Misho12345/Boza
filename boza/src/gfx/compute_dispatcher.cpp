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

        rhi::DescriptorWrite write
        {
            .binding = info.binding,
            .array_element = 0,
            .type = rhi::DescriptorType::StorageBuffer,
            .info = rhi::StorageBuffer{
                .buffer = static_cast<rhi::Buffer*>(buffer.rhi_handle()),
                .offset = 0,
                .range = static_cast<std::uint32_t>(buffer.size())
            }
        };

        desc_set->update({ &write, 1 });
        mark_set_dirty(info.set);
        return *this;
    }

    ComputeDispatcher& ComputeDispatcher::dispatch(
        const std::uint32_t width,
        const std::uint32_t height,
        const std::uint32_t depth)
    {
        std::scoped_lock lock{ mutex_ };
        if (failed_.load(std::memory_order_relaxed)) return *this;
        if (!wait_for_pending_dispatch_locked()) return *this;

        if (width == 0 || height == 0 || depth == 0)
        {
            Log::error("Dispatch dimensions must be greater than zero ({}x{}x{})", width, height, depth);
            failed_.store(true, std::memory_order_relaxed);
            return *this;
        }

        if (work_group_size_.x == 0 || work_group_size_.y == 0 || work_group_size_.z == 0)
        {
            Log::error("Invalid shader workgroup size ({}x{}x{})", work_group_size_.x, work_group_size_.y, work_group_size_.z);
            failed_.store(true, std::memory_order_relaxed);
            return *this;
        }

        const std::uint32_t group_x = (width + work_group_size_.x - 1) / work_group_size_.x;
        const std::uint32_t group_y = (height + work_group_size_.y - 1) / work_group_size_.y;
        const std::uint32_t group_z = (depth + work_group_size_.z - 1) / work_group_size_.z;

        try { dispatch_impl(group_x, group_y, group_z); }
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
        std::scoped_lock lock{ mutex_ };
        if (failed_.load(std::memory_order_relaxed)) return *this;
        if (!wait_for_pending_dispatch_locked()) return *this;

        if (x == 0 || y == 0 || z == 0)
        {
            Log::error("Dispatch group counts must be greater than zero ({}x{}x{})", x, y, z);
            failed_.store(true, std::memory_order_relaxed);
            return *this;
        }

        try { dispatch_impl(x, y, z); }
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
    }
}
