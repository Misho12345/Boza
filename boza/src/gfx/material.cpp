module boza.gfx;

import :material;
import :texture;
import :buffer;
import :sampler;
import boza.rhi;
import boza.core;
import boza.detail;
import boza.gfx.material_loader;
import boza.gfx.sampler_loader;

namespace boza
{
    struct Material::Impl
    {
        rhi::GraphicsPipeline*                 pipeline{ nullptr };
        rhi::PipelineLayout*                   pipeline_layout{ nullptr };
        std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;
        rhi::DescriptorReflection*             reflection{ nullptr };
        std::vector<rhi::DescriptorSet*>       descriptor_sets;
        std::vector<std::byte>                 push_constant_staging;

        flat_map<std::uint32_t, bool> dirty_sets;

        flat_map<std::uint32_t, std::vector<std::byte>> uniform_buffer_staging;
        flat_map<std::uint32_t, rhi::Buffer*>           uniform_buffers;

        flat_map<std::string, Sampler*> default_samplers;

        rhi::DescriptorPool* descriptor_pool{ nullptr };
    };

    Material::Material() : impl_(std::make_unique<Impl>()) { impl_->push_constant_staging.resize(128); }

    Material::~Material()
    {
        if (!impl_) return;

        if (!impl_->uniform_buffers.empty())
        {
            std::vector<rhi::Buffer*> buffers_to_delete;
            buffers_to_delete.reserve(impl_->uniform_buffers.size());

            for (auto& buffer : impl_->uniform_buffers | std::views::values)
            {
                if (buffer) buffers_to_delete.push_back(buffer);
            }

            impl_->uniform_buffers.clear();

            for (auto* buffer : buffers_to_delete)
            {
                buffer->destroy();
                delete buffer;
            }
        }

        if (impl_->descriptor_pool && !impl_->descriptor_sets.empty())
        {
            impl_->descriptor_pool->free_descriptor_sets(impl_->descriptor_sets);
        }

        delete impl_->reflection;
    }

    rhi::CompareOp to_rhi_compare_op(const CompareOp op)
    {
        switch (op)
        {
            case CompareOp::Never: return rhi::CompareOp::Never;
            case CompareOp::Less: return rhi::CompareOp::Less;
            case CompareOp::Equal: return rhi::CompareOp::Equal;
            case CompareOp::LessOrEqual: return rhi::CompareOp::LessOrEqual;
            case CompareOp::Greater: return rhi::CompareOp::Greater;
            case CompareOp::NotEqual: return rhi::CompareOp::NotEqual;
            case CompareOp::GreaterOrEqual: return rhi::CompareOp::GreaterOrEqual;
            case CompareOp::Always: return rhi::CompareOp::Always;
        }

        std::unreachable();
    }

    rhi::CullMode to_rhi_cull_mode(const CullMode mode)
    {
        switch (mode)
        {
            case CullMode::None: return rhi::CullMode::None;
            case CullMode::Front: return rhi::CullMode::Front;
            case CullMode::Back: return rhi::CullMode::Back;
            case CullMode::FrontAndBack: return rhi::CullMode::FrontAndBack;
        }

        std::unreachable();
    }

    rhi::FrontFace to_rhi_front_face(const FrontFace face)
    {
        switch (face)
        {
            case FrontFace::CounterClockwise: return rhi::FrontFace::CounterClockwise;
            case FrontFace::Clockwise: return rhi::FrontFace::Clockwise;
        }

        std::unreachable();
    }

    Material* Material::create(
        const std::string& vertex_shader_name,
        const std::string& fragment_shader_name,
        const MaterialSettings& settings)
    {
        if (!detail::RenderContext::initialized() || !detail::RenderContext::device())
        {
            Log::error("Render context not initialized. Cannot create material.");
            return nullptr;
        }

        auto* device          = detail::RenderContext::device();
        auto  api             = detail::RenderContext::api();
        auto* resource_cache  = detail::RenderContext::resource_cache();
        auto* descriptor_pool = detail::RenderContext::descriptor_pool();

        if (!resource_cache)
        {
            Log::error("ResourceCache not available in graphics context. Cannot create material.");
            return nullptr;
        }

        if (!descriptor_pool)
        {
            Log::error("DescriptorPool not available in graphics context. Cannot create material.");
            return nullptr;
        }

        const rhi::ShaderModuleDesc vert_desc{
            .device = device,
            .filename = vertex_shader_name + ".vert",
            .stage = rhi::ShaderStage::Vertex
        };

        const rhi::ShaderModuleDesc frag_desc{
            .device = device,
            .filename = fragment_shader_name + ".frag",
            .stage = rhi::ShaderStage::Fragment
        };

        const auto vert_shader_shared = resource_cache->get_or_create_shader(
            vert_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return create_shader_module(api, desc); });

        if (!vert_shader_shared)
        {
            Log::error("Failed to load vertex shader: {}", vertex_shader_name);
            return nullptr;
        }

        const auto frag_shader_shared = resource_cache->get_or_create_shader(
            frag_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return create_shader_module(api, desc); });

        if (!frag_shader_shared)
        {
            Log::error("Failed to load fragment shader: {}", fragment_shader_name);
            return nullptr;
        }

        rhi::ShaderModule* vert_shader = vert_shader_shared.get();
        rhi::ShaderModule* frag_shader = frag_shader_shared.get();

        rhi::GraphicsPipeline* pipeline        = nullptr;
        rhi::PipelineLayout*   pipeline_layout = nullptr;

        std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;

        const std::size_t settings_hash = settings.hash();
        const auto* cached = resource_cache->get_cached_pipeline(vertex_shader_name, fragment_shader_name, settings_hash);

        if (cached)
        {
            pipeline               = cached->pipeline;
            pipeline_layout        = cached->layout;
            descriptor_set_layouts = cached->descriptor_set_layouts;
        }
        else
        {
            rhi::PipelineBuilder builder(api, device, { vert_shader, frag_shader });

            if (!builder.build_descriptor_set_layouts())
            {
                Log::error("Failed to build descriptor set layouts for material");
                return nullptr;
            }

            pipeline_layout = builder.build_pipeline_layout();
            if (!pipeline_layout)
            {
                Log::error("Failed to create pipeline layout for material");
                const auto& layouts = builder.get_descriptor_set_layouts();
                for (auto* layout : layouts) { if (layout) layout->destroy(); }
                return nullptr;
            }

            const auto* swapchain = detail::RenderContext::swapchain();
            if (!swapchain)
            {
                Log::error("Swapchain not available in graphics context. Cannot create material.");
                if (pipeline_layout) pipeline_layout->destroy();
                const auto& layouts = builder.get_descriptor_set_layouts();
                for (auto* layout : layouts) { if (layout) layout->destroy(); }
                return nullptr;
            }

            const rhi::RasterizationState raster_state{
                .cull_mode = to_rhi_cull_mode(settings.cull_mode),
                .front_face = to_rhi_front_face(settings.front_face)
            };

            const rhi::DepthStencilState depth_state{
                .depth_test_enable = settings.depth_test_enable,
                .depth_write_enable = settings.depth_write_enable,
                .depth_compare_op = to_rhi_compare_op(settings.depth_compare_op)
            };

            pipeline = builder.build_graphics_pipeline(swapchain, swapchain->depth_format(), raster_state, depth_state);

            if (!pipeline)
            {
                Log::error("Failed to create graphics pipeline for material");
                if (pipeline_layout) pipeline_layout->destroy();
                const auto& layouts = builder.get_descriptor_set_layouts();
                for (auto* layout : layouts) { if (layout) layout->destroy(); }
                return nullptr;
            }

            descriptor_set_layouts = builder.get_descriptor_set_layouts();

            resource_cache->cache_pipeline(
                vertex_shader_name, fragment_shader_name, settings_hash, {
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
                Log::error("Failed to allocate descriptor set for material");
                return nullptr;
            }
            descriptor_sets.push_back(desc_set);
        }

        auto* material                          = new Material();
        material->impl_->pipeline               = pipeline;
        material->impl_->pipeline_layout        = pipeline_layout;
        material->impl_->descriptor_set_layouts = descriptor_set_layouts;
        material->impl_->descriptor_sets        = std::move(descriptor_sets);
        material->impl_->descriptor_pool        = descriptor_pool;

        auto* reflection = new rhi::DescriptorReflection();
        reflection->build_from_shaders({ vert_shader, frag_shader });
        material->impl_->reflection = reflection;

        // Log::trace("Material created with shaders: {} and {}", vertex_shader_name, fragment_shader_name);
        return material;
    }

    Material* Material::get(const std::string& name)
    {
        return gfx::MaterialLoader::instance().get_or_create_material(name);
    }

    PropertyBinder Material::operator[](const std::string_view name) { return PropertyBinder(this, std::string(name)); }

    void Material::bind() const
    {
        if (!impl_->pipeline)
        {
            Log::error("Cannot bind material: pipeline is null");
            return;
        }

        auto* cmd = detail::RenderContext::current_command_buffer();
        if (!cmd)
        {
            Log::error("Cannot bind material: no active command buffer.");
            return;
        }

        cmd->bind_graphics_pipeline(impl_->pipeline);

        if (!impl_->descriptor_sets.empty())
        {
            cmd->bind_descriptor_sets(impl_->pipeline_layout, impl_->descriptor_sets, 0);
        }
    }

    void Material::mark_set_dirty(const std::uint32_t set) const { impl_->dirty_sets[set] = true; }

    void Material::push_constants_impl(
        const std::string& name,
        const void*        data,
        const std::size_t  size,
        const ShaderDataType type) const
    {
        if (!impl_->reflection)
        {
            Log::error("Cannot push constants: reflection is null");
            return;
        }

        const auto binding_info = impl_->reflection->lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Material push constant '{}' not found in shader reflection", name);
            return;
        }

        const auto& info = binding_info.value();

        if (!info.is_push_constant)
        {
            Log::warn("Material property '{}' is not a push constant", name);
            return;
        }

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

        auto* cmd = detail::RenderContext::current_command_buffer();
        if (!cmd)
        {
            Log::error("Cannot push constants: no active command buffer");
            return;
        }

        cmd->push_constants(
            impl_->pipeline_layout,
            rhi::ShaderStage::Vertex,
            info.offset,
            static_cast<std::uint32_t>(size),
            data);
    }

    void Material::update_property_impl(
        const std::string& name,
        const void*        data,
        std::size_t        size,
        const ShaderDataType type) const
    {
        if (!impl_->reflection)
        {
            Log::error("Cannot update property: reflection is null");
            return;
        }

        const auto binding_info = impl_->reflection->lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Material property '{}' not found in shader reflection", name);
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
            if (info.offset + size <= impl_->push_constant_staging.size())
            {
                std::memcpy(impl_->push_constant_staging.data() + info.offset, data, size);
            }
            else
            {
                Log::error("Push constant '{}' offset {} + size {} exceeds staging buffer size {}",
                           name, info.offset, size, impl_->push_constant_staging.size());
            }
            return;
        }

        if (info.descriptor_type != rhi::DescriptorType::UniformBuffer)
        {
            Log::warn("Material property '{}' is not a uniform buffer member", name);
            return;
        }

        const std::uint32_t binding_key = (info.set << 16) | info.binding;

        if (!impl_->uniform_buffers.contains(binding_key))
        {
            const auto        parent_info = impl_->reflection->lookup(name.substr(0, name.find('.')));
            const std::size_t buffer_size = parent_info.has_value() ? parent_info->size : 256;

            auto* buffer = create_buffer(
                detail::RenderContext::api(), {
                    .device = detail::RenderContext::device(),
                    .size = buffer_size,
                    .usage = BufferUsage::Uniform,
                    .memory_type = rhi::BufferMemoryType::HostVisible
                });

            if (buffer)
            {
                impl_->uniform_buffers[binding_key] = buffer;
                impl_->uniform_buffer_staging[binding_key].resize(buffer_size, std::byte{ 0 });

                if (info.set < impl_->descriptor_sets.size())
                {
                    rhi::DescriptorWrite write{
                        .binding = info.binding,
                        .array_element = 0,
                        .type = rhi::DescriptorType::UniformBuffer,
                        .info = rhi::UniformBuffer{
                            .buffer = buffer,
                            .offset = 0,
                            .range = static_cast<std::uint32_t>(buffer_size)
                        }
                    };
                    impl_->descriptor_sets[info.set]->update({ write });
                }
            }
            else
            {
                Log::error("Failed to create uniform buffer for property '{}'", name);
                return;
            }
        }

        auto& staging = impl_->uniform_buffer_staging[binding_key];
        if (info.offset + size <= staging.size())
        {
            std::memcpy(staging.data() + info.offset, data, size);

            auto* buffer = impl_->uniform_buffers[binding_key];
            if (buffer) buffer->upload(staging.data(), staging.size(), 0);
        }
        else
        {
            Log::error("Uniform buffer property '{}' offset {} + size {} exceeds buffer size {}",
                       name, info.offset, size, staging.size());
        }
    }

    void Material::update_texture(const std::string& name, const Texture* texture, const Sampler* sampler) const
    {
        // If sampler not provided, use default sampler for this texture binding
        if (!sampler)
        {
            auto it = impl_->default_samplers.find(name);
            if (it != impl_->default_samplers.end()) sampler = it->second;
            else sampler                                     = gfx::SamplerLoader::instance().default_sampler();
        }

        update_texture_sampler(name, texture, sampler);
    }

    void Material::update_texture_sampler(const std::string& name, const Texture* texture, const Sampler* sampler) const
    {
        if (!impl_->reflection)
        {
            Log::error("Cannot update texture: reflection is null");
            return;
        }

        const auto binding_info = impl_->reflection->lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Material texture property '{}' not found in shader reflection", name);
            return;
        }

        const auto& info = binding_info.value();

        if (info.descriptor_type != rhi::DescriptorType::CombinedImageSampler)
        {
            Log::warn("Material property '{}' is not a combined image sampler", name);
            return;
        }

        if (sampler) impl_->default_samplers[name] = const_cast<Sampler*>(sampler);

        if (info.set < impl_->descriptor_sets.size())
        {
            rhi::DescriptorWrite write{
                .binding = info.binding,
                .array_element = 0,
                .type = rhi::DescriptorType::CombinedImageSampler,
                .info = rhi::CombinedImageSampler{
                    .texture = texture ? static_cast<rhi::Texture*>(texture->rhi_handle()) : nullptr,
                    .sampler = sampler ? static_cast<rhi::Sampler*>(sampler->rhi_handle()) : nullptr
                }
            };

            impl_->descriptor_sets[info.set]->update({ write });
            mark_set_dirty(info.set);
        }
    }

    void Material::update_buffer(const std::string& name, const Buffer* buffer) const
    {
        if (!impl_->reflection)
        {
            Log::error("Cannot update buffer: reflection is null");
            return;
        }

        const auto binding_info = impl_->reflection->lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Material buffer property '{}' not found in shader reflection", name);
            return;
        }

        const auto& info = binding_info.value();

        if (info.set < impl_->descriptor_sets.size() && buffer)
        {
            rhi::DescriptorWrite write{
                .binding = info.binding,
                .array_element = 0,
                .type = info.descriptor_type,
                .info = info.descriptor_type == rhi::DescriptorType::UniformBuffer
                            ? rhi::DescriptorInfo(rhi::UniformBuffer{
                                .buffer = static_cast<rhi::Buffer*>(buffer->rhi_handle()),
                                .offset = 0,
                                .range = static_cast<std::uint32_t>(buffer->size)
                            })
                            : rhi::DescriptorInfo(rhi::StorageBuffer{
                                .buffer = static_cast<rhi::Buffer*>(buffer->rhi_handle()),
                                .offset = 0,
                                .range = static_cast<std::uint32_t>(buffer->size)
                            })
            };

            impl_->descriptor_sets[info.set]->update({ write });
            mark_set_dirty(info.set);
        }
    }


    void* Material::rhi_pipeline_handle() const { return impl_->pipeline; }

    void* Material::rhi_pipeline_layout_handle() const
    {
        if (!impl_->pipeline) return nullptr;
        return impl_->pipeline->get_layout();
    }

    std::size_t Material::descriptor_set_count() const { return impl_->descriptor_sets.size(); }

    void* Material::rhi_descriptor_set_handle(const std::size_t index) const
    {
        if (index >= impl_->descriptor_sets.size()) return nullptr;
        return impl_->descriptor_sets[index];
    }

    std::optional<BindingInfo> Material::lookup_binding(const std::string& name) const
    {
        if (!impl_->reflection) return std::nullopt;

        auto rhi_info = impl_->reflection->lookup(name);
        if (!rhi_info.has_value()) return std::nullopt;

        const auto& [
            set, binding, offset, size,
            descriptor_type, data_type, is_push_constant
        ] = rhi_info.value();

        return BindingInfo{
            .set = set,
            .binding = binding,
            .offset = offset,
            .size = size,
            .descriptor_type = static_cast<std::uint32_t>(descriptor_type),
            .data_type = static_cast<std::uint32_t>(data_type),
            .is_push_constant = is_push_constant
        };
    }


    PropertyBinder& PropertyBinder::operator=(const Texture* texture)
    {
        material_->update_texture(name_, texture);
        return *this;
    }

    PropertyBinder& PropertyBinder::operator=(const Buffer* buffer)
    {
        material_->update_buffer(name_, buffer);
        return *this;
    }

    PropertyBinder& PropertyBinder::operator=(const std::pair<Texture*, Sampler*>& texture_sampler)
    {
        material_->update_texture_sampler(name_, texture_sampler.first, texture_sampler.second);
        return *this;
    }
}
