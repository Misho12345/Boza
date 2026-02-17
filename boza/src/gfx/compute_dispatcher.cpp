module boza.gfx;

import :compute_dispatcher;
import :texture;
import :buffer;

import boza.rhi;
import boza.rhi.render_context;

import boza.core;

namespace boza
{
    struct ComputeDispatcher::Impl
    {
        rhi::ComputePipeline* pipeline{ nullptr };
        rhi::PipelineLayout*  pipeline_layout{ nullptr };
        rhi::ShaderModule*    shader{ nullptr };
        rhi::DescriptorPool*  descriptor_pool{ nullptr };

        rhi::DescriptorReflection reflection{};

        std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;
        std::vector<rhi::DescriptorSet*>       descriptor_sets;
        std::vector<std::uint8_t>              push_constant_staging;

        flat_map<std::uint32_t, bool> dirty_sets;

        std::uint32_t push_constant_size{ 0 };
        bool          has_push_constants{ false };
    };


    ComputeDispatcher::ComputeDispatcher(const std::string& shader_name, bool& failed) : impl_{ std::make_unique<Impl>() }
    {
        failed      = false;
        failed_ptr_ = &failed;

        impl_->push_constant_staging.resize(128);

        work_group_size_ = glm::uvec3{ 1, 1, 1 };

        if (!rhi::RenderContext::initialized() || !rhi::RenderContext::device())
        {
            Log::error("Render context not initialized. Cannot create compute dispatcher.");
            failed = true;
            return;
        }

        auto  api             = rhi::RenderContext::api();
        auto* device          = rhi::RenderContext::device();
        auto* resource_cache  = rhi::RenderContext::resource_cache();
        auto* descriptor_pool = rhi::RenderContext::descriptor_pool();

        if (!resource_cache)
        {
            Log::error("ResourceCache not available in graphics context. Cannot create compute dispatcher.");
            failed = true;
            return;
        }

        if (!descriptor_pool)
        {
            Log::error("DescriptorPool not available in graphics context. Cannot create compute dispatcher.");
            failed = true;
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
            failed = true;
            return;
        }

        rhi::ShaderModule*    shader          = shader_shared.get();
        rhi::ComputePipeline* pipeline        = nullptr;
        rhi::PipelineLayout*  pipeline_layout = nullptr;
        std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;

        const auto* cached = resource_cache->get_cached_compute_pipeline(shader_name);
        if (cached)
        {
            pipeline               = cached->pipeline;
            pipeline_layout        = cached->layout;
            descriptor_set_layouts = cached->descriptor_set_layouts;
        }
        else
        {
            rhi::PipelineBuilder builder{ api, device, { shader } };

            if (!builder.build_descriptor_set_layouts())
            {
                Log::error("Failed to build descriptor set layouts for compute shader: {}", shader_name);
                failed = true;
                return;
            }

            pipeline_layout = builder.build_pipeline_layout();
            if (!pipeline_layout)
            {
                Log::error("Failed to create pipeline layout for compute shader: {}", shader_name);
                failed = true;
                return;
            }

            pipeline = builder.build_compute_pipeline();
            if (!pipeline)
            {
                Log::error("Failed to create compute pipeline for shader: {}", shader_name);
                failed = true;
                return;
            }

            descriptor_set_layouts = builder.get_descriptor_set_layouts();

            resource_cache->cache_compute_pipeline(
                shader_name,
                {
                    .pipeline = pipeline,
                    .layout = pipeline_layout,
                    .descriptor_set_layouts = descriptor_set_layouts
                });
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
                failed = true;
                return;
            }
            descriptor_sets.push_back(desc_set);
        }

        impl_->pipeline               = pipeline;
        impl_->pipeline_layout        = pipeline_layout;
        impl_->shader                 = shader;
        impl_->descriptor_set_layouts = descriptor_set_layouts;
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

        // Log::trace("ComputeDispatcher created for shader: {}", shader_name);
    }

    ComputeDispatcher::~ComputeDispatcher()
    {
        if (dispatch_started_ && pending_dispatch_.valid()) pending_dispatch_.wait();

        if (impl_->descriptor_pool && !impl_->descriptor_sets.empty())
        {
            impl_->descriptor_pool->free_descriptor_sets(impl_->descriptor_sets);
        }
    }

    // TODO: fix duplication; logic is very similar and can be shortened with a helper function
    ComputeDispatcher& ComputeDispatcher::set(const std::string& name, const Texture& texture)
    {
        if (*failed_ptr_) return *this;

        const auto binding_info = impl_->reflection.lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Compute texture '{}' not found in shader reflection", name);
            return *this;
        }

        const auto& info = binding_info.value();
        if (info.descriptor_type != rhi::DescriptorType::StorageImage)
        {
            Log::warn("Compute property '{}' is not a storage image", name);
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
            .type = rhi::DescriptorType::StorageImage,
            .info = rhi::StorageImage{
                .texture = static_cast<rhi::Texture*>(texture.rhi_handle())
            }
        };

        desc_set->update({ &write, 1 });
        mark_set_dirty(info.set);
        return *this;
    }

    ComputeDispatcher& ComputeDispatcher::set(const std::string& name, const Buffer& buffer)
    {
        if (*failed_ptr_) return *this;

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
        if (*failed_ptr_) return *this;

        const std::uint32_t group_x = 1 + (width - 1) / work_group_size_.x;
        const std::uint32_t group_y = 1 + (height - 1) / work_group_size_.y;
        const std::uint32_t group_z = 1 + (depth - 1) / work_group_size_.z;

        return dispatch_groups(group_x, group_y, group_z);
    }

    ComputeDispatcher& ComputeDispatcher::dispatch_groups(
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint32_t z)
    {
        if (*failed_ptr_) return *this;
        if (dispatch_started_ && pending_dispatch_.valid()) pending_dispatch_.wait();

        dispatch_started_ = true;
        pending_dispatch_ = std::async(std::launch::async, [this, x, y, z]
        {
            try { dispatch_impl(x, y, z); }
            catch (...)
            {
                if (failed_ptr_) *failed_ptr_ = true;
                Log::error("Compute dispatch failed with exception");
            }
        });

        return *this;
    }

    ComputeDispatcher& ComputeDispatcher::wait()
    {
        if (*failed_ptr_ || !dispatch_started_ || !pending_dispatch_.valid()) return *this;

        try { pending_dispatch_.wait(); }
        catch (...)
        {
            if (failed_ptr_) *failed_ptr_ = true;
            Log::error("Compute dispatch wait failed with exception");
        }

        return *this;
    }

    void ComputeDispatcher::dispatch_impl(const std::uint32_t x, const std::uint32_t y, const std::uint32_t z)
    {
        if (!impl_->pipeline)
        {
            Log::error("Cannot dispatch compute: pipeline is null");
            return;
        }

        const auto* device = rhi::RenderContext::device();
        if (!device)
        {
            Log::error("Cannot dispatch compute: device is null");
            return;
        }

        auto* cmd_pool = device->command_pool(device->queue_family_indices().compute_family);
        if (!cmd_pool)
        {
            Log::error("Cannot dispatch compute: compute command pool is null");
            return;
        }

        auto* cmd = cmd_pool->begin_single_time_commands();
        if (!cmd)
        {
            Log::error("Cannot dispatch compute: failed to begin command buffer");
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

        cmd_pool->end_single_time_commands(cmd);

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
        }
        else mark_set_dirty(info.set);
    }
}
