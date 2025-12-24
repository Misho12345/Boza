module boza.gfx;

import :material;
import :texture;
import :buffer;
import boza.rhi;
import boza.core;
import boza.detail;
import boza.gfx.material_loader;

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
            Log::warn("Material property '{}' type mismatch: expected {}, got {}", name, static_cast<int>(expected), static_cast<int>(actual_type));
            return false;
        }
        return true;
    }
    #endif

    struct Material::Impl
    {
        rhi::GraphicsPipeline*                  pipeline{ nullptr };
        rhi::PipelineLayout*                    pipeline_layout{ nullptr };
        std::vector<rhi::DescriptorSetLayout*>  descriptor_set_layouts;
        rhi::DescriptorReflection*              reflection{ nullptr };
        std::vector<rhi::DescriptorSet*>        descriptor_sets;
        std::vector<std::byte>                  push_constant_staging;

        flat_map<std::uint32_t, bool> dirty_sets;

        flat_map<std::uint32_t, std::vector<std::byte>> uniform_buffer_staging;
        flat_map<std::uint32_t, rhi::Buffer*>           uniform_buffers;

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
                if (buffer)
                {
                    buffers_to_delete.push_back(buffer);
                }
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
        impl_->descriptor_sets.clear();

        if (impl_->reflection)
        {
            delete impl_->reflection;
            impl_->reflection = nullptr;
        }
    }

    Material* Material::create(const std::string& vertex_shader_name, const std::string& fragment_shader_name)
    {
        if (!detail::RenderContext::initialized() || !detail::RenderContext::device())
        {
            Log::error("Render context not initialized. Cannot create material.");
            return nullptr;
        }

        auto* device          = static_cast<rhi::Device*>(detail::RenderContext::device());
        auto  api             = static_cast<rhi::GraphicsApi>(detail::RenderContext::api());
        auto* resource_cache  = static_cast<rhi::ResourceCache*>(detail::RenderContext::resource_cache());
        auto* descriptor_pool = static_cast<rhi::DescriptorPool*>(detail::RenderContext::descriptor_pool());

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
            [api](const rhi::ShaderModuleDesc& desc) { return rhi::create_shader_module(api, desc); });

        if (!vert_shader_shared)
        {
            Log::error("Failed to load vertex shader: {}", vertex_shader_name);
            return nullptr;
        }

        const auto frag_shader_shared = resource_cache->get_or_create_shader(
            frag_desc,
            [api](const rhi::ShaderModuleDesc& desc) { return rhi::create_shader_module(api, desc); });

        if (!frag_shader_shared)
        {
            Log::error("Failed to load fragment shader: {}", fragment_shader_name);
            return nullptr;
        }

        rhi::ShaderModule* vert_shader = vert_shader_shared.get();
        rhi::ShaderModule* frag_shader = frag_shader_shared.get();

        rhi::GraphicsPipeline* pipeline = nullptr;
        rhi::PipelineLayout* pipeline_layout = nullptr;
        std::vector<rhi::DescriptorSetLayout*> descriptor_set_layouts;

        const auto* cached = resource_cache->get_cached_pipeline(vertex_shader_name, fragment_shader_name);
        if (cached)
        {
            pipeline = cached->pipeline;
            pipeline_layout = cached->layout;
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

            const auto* swapchain = static_cast<rhi::Swapchain*>(detail::RenderContext::swapchain());
            if (!swapchain)
            {
                Log::error("Swapchain not available in graphics context. Cannot create material.");
                if (pipeline_layout) pipeline_layout->destroy();
                const auto& layouts = builder.get_descriptor_set_layouts();
                for (auto* layout : layouts) { if (layout) layout->destroy(); }
                return nullptr;
            }

            pipeline = builder.build_graphics_pipeline(swapchain, swapchain->depth_format());
            if (!pipeline)
            {
                Log::error("Failed to create graphics pipeline for material");
                if (pipeline_layout) pipeline_layout->destroy();
                const auto& layouts = builder.get_descriptor_set_layouts();
                for (auto* layout : layouts) { if (layout) layout->destroy(); }
                return nullptr;
            }

            descriptor_set_layouts = builder.get_descriptor_set_layouts();

            resource_cache->cache_pipeline(vertex_shader_name, fragment_shader_name, {
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

        Log::trace("Material created with shaders: {} and {}", vertex_shader_name, fragment_shader_name);
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

        auto* cmd = static_cast<rhi::CommandBuffer*>(detail::RenderContext::current_command_buffer());
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

    void Material::push_constants_impl(const std::string& name, const void* data, std::size_t size) const
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

        auto* cmd = static_cast<rhi::CommandBuffer*>(detail::RenderContext::current_command_buffer());
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

    void Material::mark_set_dirty(const std::uint32_t set) const { impl_->dirty_sets[set] = true; }

    void Material::update_property_impl(const std::string& name, const void* data, std::size_t size) const
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
            auto* device = static_cast<rhi::Device*>(detail::RenderContext::device());
            const auto api = static_cast<rhi::GraphicsApi>(detail::RenderContext::api());

            const auto parent_info = impl_->reflection->lookup(name.substr(0, name.find('.')));
            const std::size_t buffer_size = parent_info.has_value() ? parent_info->size : 256;

            auto* buffer = rhi::create_buffer(api, {
                .device = device,
                .size = buffer_size,
                .usage = BufferUsage::Uniform,
                .memory_type = rhi::BufferMemoryType::HostVisible
            });

            if (buffer)
            {
                impl_->uniform_buffers[binding_key] = buffer;
                impl_->uniform_buffer_staging[binding_key].resize(buffer_size, std::byte{0});

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
            if (buffer)
            {
                buffer->upload(staging.data(), staging.size(), 0);
            }
        }
        else
        {
            Log::error("Uniform buffer property '{}' offset {} + size {} exceeds buffer size {}",
                       name, info.offset, size, staging.size());
        }
    }

    void Material::update_texture(const std::string& name, const Texture* texture)
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

    void Material::update_buffer(const std::string& name, const Buffer* buffer)
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
        if (!impl_->reflection)
        {
            // Log::trace("lookup_binding('{}') - reflection is null", name);
            return std::nullopt;
        }

        auto rhi_info = impl_->reflection->lookup(name);
        if (!rhi_info.has_value())
        {
            // Log::trace("lookup_binding('{}') - not found in reflection", name);
            return std::nullopt;
        }

        const auto& [
            set, binding, offset, size,
            descriptor_type, data_type, is_push_constant
        ] = rhi_info.value();

        // Log::trace("lookup_binding('{}') - found: set={}, binding={}, type={}",
        //            name, set, binding, static_cast<int>(descriptor_type));

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


    PropertyBinder& PropertyBinder::operator=(Texture* texture)
    {
        material_->update_texture(name_, texture);
        return *this;
    }

    PropertyBinder& PropertyBinder::operator=(Buffer* buffer)
    {
        material_->update_buffer(name_, buffer);
        return *this;
    }
}
