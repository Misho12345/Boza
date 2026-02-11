module boza.gfx;

import :material;
import :texture;
import :buffer;
import :sampler;

import boza.core;

import boza.rhi;
import boza.rhi.render_context;

import boza.gfx.material_loader;
import boza.gfx.sampler_loader;

namespace boza
{
    Material::Material(const std::string_view name) : name_{ name } { push_constant_staging_.resize(128); }

    Material::~Material() { cleanup(); }

    void Material::cleanup()
    {
        if (!uniform_buffers_.empty())
        {
            std::vector<rhi::Buffer*> buffers_to_delete;
            buffers_to_delete.reserve(uniform_buffers_.size());

            for (auto& buffer : uniform_buffers_ | std::views::values)
            {
                if (buffer)
                {
                    buffers_to_delete.push_back(static_cast<rhi::Buffer*>(buffer));
                }
            }

            uniform_buffers_.clear();

            for (auto* buffer : buffers_to_delete)
            {
                buffer->destroy();
                delete buffer;
            }
        }

        if (descriptor_pool_ && (!descriptor_sets_.empty() || !descriptor_sets_per_frame_.empty()))
        {
            std::vector<rhi::DescriptorSet*> sets_to_free;

            if (!descriptor_sets_per_frame_.empty())
            {
                std::size_t total_set_count = 0;
                for (const auto& frame_sets : descriptor_sets_per_frame_)
                {
                    total_set_count += frame_sets.size();
                }

                sets_to_free.reserve(total_set_count);

                for (const auto& frame_sets : descriptor_sets_per_frame_)
                {
                    for (auto* set : frame_sets)
                    {
                        sets_to_free.push_back(static_cast<rhi::DescriptorSet*>(set));
                    }
                }
            }
            else
            {
                sets_to_free.reserve(descriptor_sets_.size());

                for (auto* set : descriptor_sets_)
                {
                    sets_to_free.push_back(static_cast<rhi::DescriptorSet*>(set));
                }
            }

            static_cast<rhi::DescriptorPool*>(descriptor_pool_)->free_descriptor_sets(sets_to_free);
        }

        descriptor_sets_.clear();
        descriptor_sets_per_frame_.clear();

        if (reflection_)
        {
            delete static_cast<rhi::DescriptorReflection*>(reflection_);
            reflection_ = nullptr;
        }
    }

    Material::Material(Material&& other) noexcept
        : name_{ std::move(other.name_) },
          pipeline_{ std::exchange(other.pipeline_, nullptr) },
          pipeline_layout_{ std::exchange(other.pipeline_layout_, nullptr) },
          descriptor_set_layouts_{ std::move(other.descriptor_set_layouts_) },
          descriptor_sets_{ std::move(other.descriptor_sets_) },
          descriptor_sets_per_frame_{ std::move(other.descriptor_sets_per_frame_) },
          push_constant_staging_{ std::move(other.push_constant_staging_) },
          reflection_{ std::exchange(other.reflection_, nullptr) },
          dirty_sets_{ std::move(other.dirty_sets_) },
          uniform_buffer_staging_{ std::move(other.uniform_buffer_staging_) },
          uniform_buffers_{ std::move(other.uniform_buffers_) },
          default_samplers_{ std::move(other.default_samplers_) },
          descriptor_pool_{ std::exchange(other.descriptor_pool_, nullptr) },
          cpu_cull_enabled_{ std::exchange(other.cpu_cull_enabled_, true) } {}

    Material& Material::operator=(Material&& other) noexcept
    {
        if (this == &other) return *this;

        cleanup();

        name_ = std::move(other.name_);
        pipeline_ = std::exchange(other.pipeline_, nullptr);
        pipeline_layout_ = std::exchange(other.pipeline_layout_, nullptr);
        descriptor_set_layouts_ = std::move(other.descriptor_set_layouts_);
        descriptor_sets_ = std::move(other.descriptor_sets_);
        descriptor_sets_per_frame_ = std::move(other.descriptor_sets_per_frame_);
        push_constant_staging_ = std::move(other.push_constant_staging_);
        reflection_ = std::exchange(other.reflection_, nullptr);
        dirty_sets_ = std::move(other.dirty_sets_);
        uniform_buffer_staging_ = std::move(other.uniform_buffer_staging_);
        uniform_buffers_ = std::move(other.uniform_buffers_);
        default_samplers_ = std::move(other.default_samplers_);
        descriptor_pool_ = std::exchange(other.descriptor_pool_, nullptr);
        cpu_cull_enabled_ = std::exchange(other.cpu_cull_enabled_, true);

        return *this;
    }

    Material& Material::create(
        const std::string_view name,
        const MaterialSettings& settings)
    {
        return gfx::MaterialLoader::instance().get_or_create_material(name, settings);
    }

    Material& Material::get(const std::string_view name)
    {
        auto* material = try_get(name);
        assert(material != nullptr, "Material not found");
        return *material;
    }

    Material* Material::try_get(const std::string_view name)
    {
        return gfx::MaterialLoader::instance().try_get_material(name);
    }

    void Material::destroy(std::string_view name)
    {
        gfx::MaterialLoader::instance().destroy(name);
    }

    PropertyBinder Material::operator[](const std::string_view name) { return PropertyBinder(this, std::string(name)); }

    void Material::bind() const
    {
        if (!pipeline_)
        {
            Log::error("Cannot bind material: pipeline is null");
            return;
        }

        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!cmd)
        {
            Log::error("Cannot bind material: no active command buffer.");
            return;
        }

        cmd->bind_graphics_pipeline(static_cast<rhi::GraphicsPipeline*>(pipeline_));

        const std::vector<void*>* active_descriptor_sets = &descriptor_sets_;

        if (!descriptor_sets_per_frame_.empty())
        {
            std::uint32_t frame_index = 0;

            if (const auto* swapchain = rhi::RenderContext::swapchain())
            {
                frame_index = swapchain->current_frame();
            }

            frame_index %= static_cast<std::uint32_t>(descriptor_sets_per_frame_.size());
            active_descriptor_sets = &descriptor_sets_per_frame_[frame_index];
        }

        if (active_descriptor_sets && !active_descriptor_sets->empty())
        {
            std::vector<rhi::DescriptorSet*> sets;
            sets.reserve(active_descriptor_sets->size());
            for (auto* set : *active_descriptor_sets)
            {
                sets.push_back(static_cast<rhi::DescriptorSet*>(set));
            }
            cmd->bind_descriptor_sets(static_cast<rhi::PipelineLayout*>(pipeline_layout_), sets, 0);
        }
    }

    void Material::mark_set_dirty(const std::uint32_t set) { dirty_sets_[set] = true; }

    void Material::push_constants(
        const std::string& name,
        const void*        data,
        const std::size_t  size,
        [[maybe_unused]] const ShaderDataType type) const
    {
        if (!reflection_)
        {
            Log::error("Cannot push constants: reflection is null");
            return;
        }

        const auto binding_info = static_cast<rhi::DescriptorReflection*>(reflection_)->lookup(name);
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

        auto* cmd = rhi::RenderContext::current_command_buffer();
        if (!cmd)
        {
            Log::error("Cannot push constants: no active command buffer");
            return;
        }

        cmd->push_constants(
            static_cast<rhi::PipelineLayout*>(pipeline_layout_),
            rhi::ShaderStage::Vertex,
            info.offset,
            static_cast<std::uint32_t>(size),
            data);
    }

    void Material::update_property(
        const std::string& name,
        const void*        data,
        std::size_t        size,
        [[maybe_unused]] const ShaderDataType type)
    {
        if (!reflection_)
        {
            Log::error("Cannot update property: reflection is null");
            return;
        }

        const auto binding_info = static_cast<rhi::DescriptorReflection*>(reflection_)->lookup(name);
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
            if (info.offset + size <= push_constant_staging_.size())
            {
                std::memcpy(push_constant_staging_.data() + info.offset, data, size);
            }
            else
            {
                Log::error("Push constant '{}' offset {} + size {} exceeds staging buffer size {}",
                           name, info.offset, size, push_constant_staging_.size());
            }
            return;
        }

        if (info.descriptor_type != rhi::DescriptorType::UniformBuffer)
        {
            Log::warn("Material property '{}' is not a uniform buffer member", name);
            return;
        }

        const std::uint32_t binding_key = (info.set << 16) | info.binding;

        if (!uniform_buffers_.contains(binding_key))
        {
            const auto        parent_info = static_cast<rhi::DescriptorReflection*>(reflection_)->lookup(name.substr(0, name.find('.')));
            const std::size_t buffer_size = parent_info.has_value() ? parent_info->size : 256;

            auto* buffer = create_buffer(
                rhi::RenderContext::api(), {
                    .device = rhi::RenderContext::device(),
                    .size = buffer_size,
                    .usage = BufferUsage::Uniform,
                    .memory_type = rhi::BufferMemoryType::HostVisible
                });

            if (buffer)
            {
                uniform_buffers_[binding_key] = buffer;
                uniform_buffer_staging_[binding_key].resize(buffer_size, 0);

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

                if (!descriptor_sets_per_frame_.empty())
                {
                    for (const auto& frame_sets : descriptor_sets_per_frame_)
                    {
                        if (info.set >= frame_sets.size()) continue;
                        static_cast<rhi::DescriptorSet*>(frame_sets[info.set])->update({ &write, 1 });
                    }
                }
                else if (info.set < descriptor_sets_.size())
                {
                    static_cast<rhi::DescriptorSet*>(descriptor_sets_[info.set])->update({ &write, 1 });
                }
            }
            else
            {
                Log::error("Failed to create uniform buffer for property '{}'", name);
                return;
            }
        }

        auto& staging = uniform_buffer_staging_[binding_key];
        if (info.offset + size <= staging.size())
        {
            std::memcpy(staging.data() + info.offset, data, size);

            auto* buffer = static_cast<rhi::Buffer*>(uniform_buffers_[binding_key]);
            if (buffer) buffer->upload(staging.data(), staging.size(), 0);
        }
        else
        {
            Log::error("Uniform buffer property '{}' offset {} + size {} exceeds buffer size {}",
                       name, info.offset, size, staging.size());
        }
    }

    void Material::update_texture(const std::string& name, const Texture& texture)
    {
        update_texture(name, texture, gfx::SamplerLoader::instance().default_sampler());
    }

    void Material::update_texture(const std::string& name, const Texture& texture, Sampler& sampler)
    {
        if (!reflection_)
        {
            Log::error("Cannot update texture: reflection is null");
            return;
        }

        const auto binding_info = static_cast<rhi::DescriptorReflection*>(reflection_)->lookup(name);
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

        default_samplers_[name] = &sampler;

        rhi::DescriptorWrite write{
            .binding = info.binding,
            .array_element = 0,
            .type = rhi::DescriptorType::CombinedImageSampler,
            .info = rhi::CombinedImageSampler{
                .texture = static_cast<rhi::Texture*>(texture.rhi_handle()),
                .sampler = static_cast<rhi::Sampler*>(sampler.rhi_handle())
            }
        };

        if (!descriptor_sets_per_frame_.empty())
        {
            for (const auto& frame_sets : descriptor_sets_per_frame_)
            {
                if (info.set >= frame_sets.size()) continue;
                static_cast<rhi::DescriptorSet*>(frame_sets[info.set])->update({ &write, 1 });
            }

            mark_set_dirty(info.set);
        }
        else if (info.set < descriptor_sets_.size())
        {
            static_cast<rhi::DescriptorSet*>(descriptor_sets_[info.set])->update({ &write, 1 });
            mark_set_dirty(info.set);
        }
    }

    void Material::update_buffer(const std::string& name, const Buffer& buffer)
    {
        if (!reflection_)
        {
            Log::error("Cannot update buffer: reflection is null");
            return;
        }

        const auto binding_info = static_cast<rhi::DescriptorReflection*>(reflection_)->lookup(name);
        if (!binding_info.has_value())
        {
            Log::warn("Material buffer property '{}' not found in shader reflection", name);
            return;
        }

        const auto& info = binding_info.value();

        const auto update_set = [&buffer, &info](void* set_handle, const std::uint32_t frame_index) -> bool
        {
            auto* buffer_handle = static_cast<rhi::Buffer*>(buffer.rhi_handle(frame_index));
            if (!buffer_handle) return false;

            rhi::DescriptorWrite write{
                .binding = info.binding,
                .array_element = 0,
                .type = info.descriptor_type,
                .info = info.descriptor_type == rhi::DescriptorType::UniformBuffer
                            ? rhi::DescriptorInfo(rhi::UniformBuffer{
                                .buffer = buffer_handle,
                                .offset = 0,
                                .range = static_cast<std::uint32_t>(buffer.size())
                            })
                            : rhi::DescriptorInfo(rhi::StorageBuffer{
                                .buffer = buffer_handle,
                                .offset = 0,
                                .range = static_cast<std::uint32_t>(buffer.size())
                            })
            };

            static_cast<rhi::DescriptorSet*>(set_handle)->update({ &write, 1 });
            return true;
        };

        const bool has_per_frame_sets = !descriptor_sets_per_frame_.empty();
        const bool dynamic_buffer = buffer.access_mode == ResourceAccessMode::Dynamic;

        if (has_per_frame_sets)
        {
            if (dynamic_buffer)
            {
                std::uint32_t frame_index = 0;
                if (const auto* swapchain = rhi::RenderContext::swapchain())
                {
                    frame_index = swapchain->current_frame();
                }

                frame_index %= static_cast<std::uint32_t>(descriptor_sets_per_frame_.size());

                const auto& frame_sets = descriptor_sets_per_frame_[frame_index];
                if (info.set < frame_sets.size() && !update_set(frame_sets[info.set], frame_index))
                {
                    Log::error("Cannot update dynamic buffer '{}': invalid RHI buffer handle for frame {}", name, frame_index);
                    return;
                }
            }
            else
            {
                for (const auto& frame_sets : descriptor_sets_per_frame_)
                {
                    if (info.set >= frame_sets.size()) continue;
                    if (!update_set(frame_sets[info.set], 0))
                    {
                        Log::error("Cannot update buffer '{}': invalid static RHI buffer handle", name);
                        return;
                    }
                }
            }

            mark_set_dirty(info.set);
            return;
        }

        if (info.set < descriptor_sets_.size())
        {
            const std::uint32_t frame_index = dynamic_buffer ? 0u : 0u;

            if (!update_set(descriptor_sets_[info.set], frame_index))
            {
                Log::error("Cannot update buffer '{}': invalid RHI buffer handle", name);
                return;
            }

            mark_set_dirty(info.set);
        }
    }


    std::size_t Material::descriptor_set_count() const { return descriptor_sets_.size(); }

    void* Material::rhi_descriptor_set_handle(const std::size_t index) const
    {
        if (index >= descriptor_sets_.size()) return nullptr;
        return descriptor_sets_[index];
    }

    std::optional<BindingInfo> Material::lookup_binding(const std::string& name) const
    {
        if (!reflection_) return std::nullopt;

        auto rhi_info = static_cast<rhi::DescriptorReflection*>(reflection_)->lookup(name);
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


    PropertyBinder& PropertyBinder::operator=(const Texture& texture)
    {
        material_->update_texture(name_, texture);
        return *this;
    }

    PropertyBinder& PropertyBinder::operator=(const Buffer& buffer)
    {
        material_->update_buffer(name_, buffer);
        return *this;
    }

    PropertyBinder& PropertyBinder::operator=(const std::pair<Texture&, Sampler&> texture_sampler)
    {
        material_->update_texture(name_, texture_sampler.first, texture_sampler.second);
        return *this;
    }
}
