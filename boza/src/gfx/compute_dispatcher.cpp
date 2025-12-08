module boza.gfx;

import :compute_dispatcher;
import :texture;
import :buffer;
import boza.rhi;
import boza.core;
import boza.detail;

namespace boza
{
    #ifdef BOZA_DEBUG
    template<typename T>
    constexpr rhi::ShaderDataType get_expected_shader_type()
    {
        if constexpr (std::is_same_v<T, float>) return rhi::ShaderDataType::Float;
        else if constexpr (std::is_same_v<T, std::int32_t>) return rhi::ShaderDataType::Int;
        else if constexpr (std::is_same_v<T, std::uint32_t>) return rhi::ShaderDataType::Uint;
        else if constexpr (std::is_same_v<T, glm::vec2>) return rhi::ShaderDataType::Vec2;
        else if constexpr (std::is_same_v<T, glm::vec3>) return rhi::ShaderDataType::Vec3;
        else if constexpr (std::is_same_v<T, glm::vec4>) return rhi::ShaderDataType::Vec4;
        else if constexpr (std::is_same_v<T, glm::ivec2>) return rhi::ShaderDataType::IVec2;
        else if constexpr (std::is_same_v<T, glm::ivec3>) return rhi::ShaderDataType::IVec3;
        else if constexpr (std::is_same_v<T, glm::ivec4>) return rhi::ShaderDataType::IVec4;
        else if constexpr (std::is_same_v<T, glm::uvec2>) return rhi::ShaderDataType::UVec2;
        else if constexpr (std::is_same_v<T, glm::uvec3>) return rhi::ShaderDataType::UVec3;
        else if constexpr (std::is_same_v<T, glm::uvec4>) return rhi::ShaderDataType::UVec4;
        else if constexpr (std::is_same_v<T, glm::mat2>) return rhi::ShaderDataType::Mat2;
        else if constexpr (std::is_same_v<T, glm::mat3>) return rhi::ShaderDataType::Mat3;
        else if constexpr (std::is_same_v<T, glm::mat4>) return rhi::ShaderDataType::Mat4;
        else return rhi::ShaderDataType::Unknown;
    }

    template<typename T>
    bool validate_property_type(const std::string& name, rhi::ShaderDataType actual_type)
    {
        const auto expected = get_expected_shader_type<T>();
        if (expected != rhi::ShaderDataType::Unknown && expected != actual_type)
        {
            Log::warn("Compute property '{}' type mismatch: expected {}, got {}",
                      name, static_cast<int>(expected), static_cast<int>(actual_type));
            return false;
        }
        return true;
    }
    #endif

    struct ComputeDispatcher::Impl
    {
        rhi::ComputePipeline*                   pipeline{ nullptr };
        rhi::PipelineLayout*                    pipeline_layout{ nullptr };
        rhi::ShaderModule*                      shader{ nullptr };
        rhi::DescriptorReflection*              reflection{ nullptr };
        std::vector<rhi::DescriptorSetLayout*>  descriptor_set_layouts;
        std::vector<rhi::DescriptorSet*>        descriptor_sets;
        std::vector<std::byte>                  push_constant_staging;
        std::unordered_map<std::uint32_t, bool> dirty_sets;
    };

    ComputeDispatcher::ComputeDispatcher() : impl_(std::make_unique<Impl>())
    {
        impl_->push_constant_staging.resize(128);
    }

    ComputeDispatcher::~ComputeDispatcher()
    {
        if (impl_)
        {
            if (impl_->pipeline)
            {
                impl_->pipeline->destroy();
                impl_->pipeline = nullptr;
            }

            if (impl_->pipeline_layout)
            {
                impl_->pipeline_layout->destroy();
                impl_->pipeline_layout = nullptr;
            }

            for (auto* desc_set : impl_->descriptor_sets) { if (desc_set) desc_set->destroy(); }

            impl_->descriptor_sets.clear();

            for (auto* layout : impl_->descriptor_set_layouts) { if (layout) layout->destroy(); }

            impl_->descriptor_set_layouts.clear();

            if (impl_->reflection)
            {
                delete impl_->reflection;
                impl_->reflection = nullptr;
            }
        }
    }

    ComputeDispatcher* ComputeDispatcher::create(const std::string& shader_name)
    {
        if (!detail::RenderContext::initialized() || !detail::RenderContext::device())
        {
            Log::error("Render context not initialized. Cannot create compute dispatcher.");
            return nullptr;
        }

        auto* device          = static_cast<rhi::Device*>(detail::RenderContext::device());
        auto  api             = static_cast<rhi::GraphicsApi>(detail::RenderContext::api());
        auto* resource_cache  = static_cast<rhi::ResourceCache*>(detail::RenderContext::resource_cache());
        auto* descriptor_pool = static_cast<rhi::DescriptorPool*>(detail::RenderContext::descriptor_pool());

        if (!resource_cache)
        {
            Log::error("ResourceCache not available in graphics context. Cannot create compute dispatcher.");
            return nullptr;
        }

        if (!descriptor_pool)
        {
            Log::error("DescriptorPool not available in graphics context. Cannot create compute dispatcher.");
            return nullptr;
        }

        const rhi::ShaderModuleDesc shader_desc
        {
            .device = device,
            .filename = shader_name + ".comp",
            .stage = rhi::ShaderStage::Compute
        };

        const auto shader_shared = resource_cache->get_or_create_shader(
            shader_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return rhi::create_shader_module(api, desc); });

        if (!shader_shared)
        {
            Log::error("Failed to load compute shader: {}", shader_name);
            return nullptr;
        }

        rhi::ShaderModule* shader = shader_shared.get();

        rhi::PipelineBuilder builder(api, device, { shader });

        if (!builder.build_descriptor_set_layouts())
        {
            Log::error("Failed to build descriptor set layouts for compute shader: {}", shader_name);
            return nullptr;
        }

        rhi::PipelineLayout* pipeline_layout = builder.build_pipeline_layout();
        if (!pipeline_layout)
        {
            Log::error("Failed to create pipeline layout for compute shader: {}", shader_name);
            return nullptr;
        }

        rhi::ComputePipeline* pipeline = builder.build_compute_pipeline();
        if (!pipeline)
        {
            Log::error("Failed to create compute pipeline for shader: {}", shader_name);
            if (pipeline_layout) pipeline_layout->destroy();
            return nullptr;
        }

        const auto&                      descriptor_set_layouts = builder.get_descriptor_set_layouts();
        std::vector<rhi::DescriptorSet*> descriptor_sets;
        descriptor_sets.reserve(descriptor_set_layouts.size());

        for (auto* layout : descriptor_set_layouts)
        {
            auto* desc_set = descriptor_pool->allocate_descriptor_set(layout);
            if (!desc_set)
            {
                Log::error("Failed to allocate descriptor set for compute dispatcher");

                for (auto* set : descriptor_sets) { if (set) set->destroy(); }
                if (pipeline) pipeline->destroy();
                if (pipeline_layout) pipeline_layout->destroy();
                return nullptr;
            }
            descriptor_sets.push_back(desc_set);
        }

        auto* dispatcher                          = new ComputeDispatcher();
        dispatcher->impl_->pipeline               = pipeline;
        dispatcher->impl_->pipeline_layout        = pipeline_layout;
        dispatcher->impl_->shader                 = shader;
        dispatcher->impl_->descriptor_set_layouts = descriptor_set_layouts;
        dispatcher->impl_->descriptor_sets        = std::move(descriptor_sets);

        auto* reflection = new rhi::DescriptorReflection();
        reflection->build_from_shaders({ shader });
        dispatcher->impl_->reflection = reflection;

        dispatcher->work_group_size_ = shader->meta_data().work_group_size;

        Log::trace("ComputeDispatcher created for shader: {}", shader_name);
        return dispatcher;
    }

    ComputePropertyBinder ComputeDispatcher::operator[](const std::string_view property_name)
    {
        return ComputePropertyBinder(this, std::string(property_name));
    }

    void ComputeDispatcher::dispatch(const std::uint32_t width, const std::uint32_t height, const std::uint32_t depth)
    {
        const std::uint32_t group_x = (width + work_group_size_.x - 1) / work_group_size_.x;
        const std::uint32_t group_y = (height + work_group_size_.y - 1) / work_group_size_.y;
        const std::uint32_t group_z = (depth + work_group_size_.z - 1) / work_group_size_.z;

        dispatch_groups(group_x, group_y, group_z);
    }

    void ComputeDispatcher::dispatch_groups(const std::uint32_t x, const std::uint32_t y, const std::uint32_t z)
    {
        if (!impl_->pipeline)
        {
            Log::error("Cannot dispatch compute: pipeline is null");
            return;
        }

        const auto* device = static_cast<rhi::Device*>(detail::RenderContext::device());
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

        cmd->dispatch(x, y, z);

        cmd_pool->end_single_time_commands(cmd);

        impl_->dirty_sets.clear();
    }

    void ComputeDispatcher::mark_set_dirty(const std::uint32_t set) { impl_->dirty_sets[set] = true; }

    template<typename T>
    void ComputeDispatcher::update_property(const std::string& name, const T& value)
    {
        if (!impl_->reflection)
        {
            Log::error("Cannot update property: reflection is null");
            return;
        }

        const auto binding_info = impl_->reflection->lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Compute property '{}' not found in shader reflection", name);
            return;
        }

        const auto& info = binding_info.value();

        #ifdef BOZA_DEBUG
        validate_property_type<T>(name, info.data_type);
        #endif

        if (info.is_push_constant)
        {
            if (info.offset + sizeof(T) <= impl_->push_constant_staging.size())
            {
                std::memcpy(impl_->push_constant_staging.data() + info.offset, &value, sizeof(T));
            }
            else
            {
                Log::error("Push constant '{}' offset {} + size {} exceeds staging buffer size {}",
                           name, info.offset, sizeof(T), impl_->push_constant_staging.size());
            }
        }
        else mark_set_dirty(info.set);
    }

    void ComputeDispatcher::update_texture(const std::string& name, Texture* texture)
    {
        if (!impl_->reflection)
        {
            Log::error("Cannot update texture: reflection is null");
            return;
        }

        const auto binding_info = impl_->reflection->lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Compute texture property '{}' not found in shader reflection", name);
            return;
        }

        const auto& info = binding_info.value();

        if (info.descriptor_type == rhi::DescriptorType::StorageImage)
        {
            if (info.set < impl_->descriptor_sets.size())
            {
                rhi::DescriptorWrite write{
                    .binding = info.binding,
                    .array_element = 0,
                    .type = rhi::DescriptorType::StorageImage,
                    .info = rhi::StorageImage{
                        .texture = texture ? static_cast<rhi::Texture*>(texture->rhi_handle()) : nullptr
                    }
                };

                impl_->descriptor_sets[info.set]->update({ write });
                mark_set_dirty(info.set);
            }
        }
        else if (info.descriptor_type == rhi::DescriptorType::CombinedImageSampler)
        {
            if (info.set < impl_->descriptor_sets.size())
            {
                rhi::DescriptorWrite write{
                    .binding = info.binding,
                    .array_element = 0,
                    .type = rhi::DescriptorType::CombinedImageSampler,
                    .info = rhi::CombinedImageSampler{
                        .sampler = texture ? static_cast<rhi::Sampler*>(texture->rhi_sampler_handle()) : nullptr,
                        .texture = texture ? static_cast<rhi::Texture*>(texture->rhi_handle()) : nullptr
                    }
                };

                impl_->descriptor_sets[info.set]->update({ write });
                mark_set_dirty(info.set);
            }
        }
        else Log::warn("Compute property '{}' is not a texture/image type", name);
    }

    void ComputeDispatcher::update_buffer(const std::string& name, Buffer* buffer)
    {
        if (!impl_->reflection)
        {
            Log::error("Cannot update buffer: reflection is null");
            return;
        }

        const auto binding_info = impl_->reflection->lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Compute buffer property '{}' not found in shader reflection", name);
            return;
        }

        const auto& info = binding_info.value();

        if (info.set < impl_->descriptor_sets.size() && buffer)
        {
            rhi::DescriptorWrite write{
                .binding = info.binding,
                .array_element = 0,
                .type = info.descriptor_type,
                .info = rhi::StorageBuffer{
                    .buffer = static_cast<rhi::Buffer*>(buffer->rhi_handle()),
                    .offset = 0,
                    .range = static_cast<std::uint32_t>(buffer->size)
                }
            };

            impl_->descriptor_sets[info.set]->update({ write });
            mark_set_dirty(info.set);
        }
    }

    ComputePropertyBinder& ComputePropertyBinder::operator=(Texture* texture)
    {
        dispatcher_->update_texture(name_, texture);
        return *this;
    }

    ComputePropertyBinder& ComputePropertyBinder::operator=(Buffer* buffer)
    {
        dispatcher_->update_buffer(name_, buffer);
        return *this;
    }

    template void ComputeDispatcher::update_property(const std::string&, const float&);
    template void ComputeDispatcher::update_property(const std::string&, const double&);
    template void ComputeDispatcher::update_property(const std::string&, const std::int32_t&);
    template void ComputeDispatcher::update_property(const std::string&, const std::uint32_t&);
    template void ComputeDispatcher::update_property(const std::string&, const bool&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::vec2&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::vec3&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::vec4&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::ivec2&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::ivec3&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::ivec4&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::uvec2&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::uvec3&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::uvec4&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::mat2&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::mat3&);
    template void ComputeDispatcher::update_property(const std::string&, const glm::mat4&);
}
