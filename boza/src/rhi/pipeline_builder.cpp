module boza.rhi;

import boza.core;
import boza.detail;
import :pipeline_builder;

namespace boza::rhi
{
    using detail::AssetPaths;

     /// ----------------------------
    /// ===== Pipeline Builder =====
    /// ----------------------------

    PipelineBuilder::PipelineBuilder(const GraphicsApi api, Device* device, const std::vector<ShaderModule*>& shaders)
        : api_(api),
          device_(device),
          shaders_(shaders) {}

    std::unordered_map<std::uint32_t, std::vector<PipelineBuilder::DescriptorBinding>>
    PipelineBuilder::merge_descriptor_bindings() const
    {
        std::unordered_map<std::uint32_t, std::vector<DescriptorBinding>> bindings_by_set;

        for (const auto* shader : shaders_)
        {
            const auto& metadata   = shader->meta_data();
            const auto  stage_flag = Flags(shader->stage());

            for (const auto& resource : metadata.uniform_buffers | std::views::values)
            {
                auto& bindings = bindings_by_set[resource.set];

                auto it = std::ranges::find_if(bindings, [&](const DescriptorBinding& b)
                {
                    return b.binding == resource.binding;
                });

                if (it != bindings.end()) it->stages |= stage_flag;
                else
                {
                    bindings.emplace_back(
                        resource.binding,
                        DescriptorType::UniformBuffer,
                        stage_flag,
                        1
                    );
                }
            }

            for (const auto& resource : metadata.storage_buffers | std::views::values)
            {
                auto& bindings = bindings_by_set[resource.set];

                auto it = std::ranges::find_if(bindings, [&](const DescriptorBinding& b)
                {
                    return b.binding == resource.binding;
                });

                if (it != bindings.end()) { it->stages |= stage_flag; }
                else
                {
                    bindings.emplace_back(
                        resource.binding,
                        DescriptorType::StorageBuffer,
                        stage_flag,
                        1
                    );
                }
            }

            for (const auto& resource : metadata.sampled_images | std::views::values)
            {
                auto& bindings = bindings_by_set[resource.set];

                auto it = std::ranges::find_if(bindings, [&](const DescriptorBinding& b)
                {
                    return b.binding == resource.binding;
                });

                if (it != bindings.end()) { it->stages |= stage_flag; }
                else
                {
                    bindings.emplace_back(
                        resource.binding,
                        DescriptorType::CombinedImageSampler,
                        stage_flag,
                        1
                    );
                }
            }

            for (const auto& resource : metadata.storage_images | std::views::values)
            {
                auto& bindings = bindings_by_set[resource.set];

                auto it = std::ranges::find_if(bindings, [&](const DescriptorBinding& b)
                {
                    return b.binding == resource.binding;
                });

                if (it != bindings.end()) { it->stages |= stage_flag; }
                else
                {
                    bindings.emplace_back(
                        resource.binding,
                        DescriptorType::StorageImage,
                        stage_flag,
                        1
                    );
                }
            }
        }

        return bindings_by_set;
    }

    bool PipelineBuilder::build_descriptor_set_layouts()
    {
        auto bindings_by_set = merge_descriptor_bindings();

        for (auto* layout : descriptor_set_layouts_)
        {
            layout->destroy();
            delete layout;
        }

        descriptor_set_layouts_.clear();

        if (bindings_by_set.empty())
        {
            // Log::trace("No descriptor bindings found in shaders");
            return true;
        }

        std::uint32_t max_set = 0;
        for (const auto& set_idx : bindings_by_set | std::views::keys) max_set = std::max(max_set, set_idx);

        for (std::uint32_t set_idx = 0; set_idx <= max_set; ++set_idx)
        {
            std::vector<DescriptorSetLayoutBinding> layout_bindings;

            if (bindings_by_set.contains(set_idx))
            {
                const auto& bindings = bindings_by_set[set_idx];
                layout_bindings.reserve(bindings.size());

                for (const auto& [binding, type, stages, count] : bindings)
                    layout_bindings.emplace_back(
                        binding, type, stages, count);
            }

            auto* layout = create_descriptor_set_layout(
                api_, {
                    .device = device_,
                    .bindings = layout_bindings
                });

            if (!layout)
            {
                Log::error("Failed to create descriptor set layout for set {}", set_idx);
                return false;
            }

            descriptor_set_layouts_.push_back(layout);
        }

        // Log::trace("Created {} descriptor set layout(s)", descriptor_set_layouts_.size());
        return true;
    }

    PipelineLayout* PipelineBuilder::build_pipeline_layout()
    {
        pipeline_layout_ = create_pipeline_layout(
            api_, {
                .device = device_,
                .shaders = shaders_,
                .set_layouts = descriptor_set_layouts_
            });

        if (!pipeline_layout_)
        {
            Log::error("Failed to create pipeline layout");
            return nullptr;
        }

        // Log::trace("Created pipeline layout with {} descriptor set(s)", descriptor_set_layouts_.size());
        return pipeline_layout_;
    }

    GraphicsPipeline* PipelineBuilder::build_graphics_pipeline(
        const std::vector<std::uint32_t>& color_attachment_formats,
        const DepthFormat                 depth_attachment_format,
        const RasterizationState&    rasterization,
        const DepthStencilState&     depth_stencil,
        const ColorBlendState&       color_blend,
        const PrimitiveTopology      topology) const
    {
        if (!pipeline_layout_)
        {
            Log::error("Pipeline layout not created. Call build_pipeline_layout() first.");
            return nullptr;
        }

        ColorBlendState adjusted_color_blend = color_blend;
        if (adjusted_color_blend.attachments.empty() && !color_attachment_formats.empty())
        {
            adjusted_color_blend.attachments.resize(color_attachment_formats.size());
        }
        else if (adjusted_color_blend.attachments.size() != color_attachment_formats.size())
        {
            Log::warn("Color blend attachment count ({}) doesn't match color attachment format count ({}). Adjusting...",
                adjusted_color_blend.attachments.size(), color_attachment_formats.size());
            adjusted_color_blend.attachments.resize(color_attachment_formats.size());
        }

        std::vector<VertexInputBinding>   bindings;
        std::vector<VertexInputAttribute> attributes;

        const ShaderModule* vertex_shader = nullptr;
        for (const auto* shader : shaders_)
        {
            if (shader->stage() == ShaderStage::Vertex)
            {
                vertex_shader = shader;
                break;
            }
        }

        if (vertex_shader)
        {
            const auto& metadata = vertex_shader->meta_data();
            std::uint32_t total_stride = 0;

            for (const auto& input : metadata.stage_inputs | std::views::values) total_stride += input.size;

            if (total_stride > 0)
            {
                bindings.emplace_back(0, total_stride, false);

                std::vector<std::pair<std::uint32_t, ShaderModule::ShaderResource>> sorted_inputs;
                for (const auto& input : metadata.stage_inputs | std::views::values)
                    sorted_inputs.emplace_back(input.location, input);

                std::ranges::sort(sorted_inputs, [](const auto& a, const auto& b) { return a.first < b.first; });

                std::uint32_t offset = 0;
                for (const auto& [location, input] : sorted_inputs)
                {
                    attributes.emplace_back(location, 0, offset);
                    offset += input.size;
                }

                // Log::trace("Configured vertex input: {} attributes, stride = {}", attributes.size(), total_stride);
            }
        }

        auto* pipeline = create_graphics_pipeline(
            api_, {
                .device = device_,
                .shaders = shaders_,
                .layout = pipeline_layout_,
                .topology = topology,
                .bindings = bindings,
                .attributes = attributes,
                .rasterization = rasterization,
                .depth_stencil = depth_stencil,
                .color_blend = adjusted_color_blend,
                .color_attachment_formats = color_attachment_formats,
                .depth_attachment_format = depth_attachment_format,
                .stencil_attachment_format = 0
            });

        if (!pipeline)
        {
            Log::error("Failed to create graphics pipeline");
            return nullptr;
        }

        // Log::trace("Created graphics pipeline");
        return pipeline;
    }

    GraphicsPipeline* PipelineBuilder::build_graphics_pipeline(
        const Swapchain*          swapchain,
        const DepthFormat         depth_attachment_format,
        const RasterizationState& rasterization,
        const DepthStencilState&  depth_stencil,
        const ColorBlendState&    color_blend,
        const PrimitiveTopology   topology) const
    {
        return build_graphics_pipeline(
            { swapchain->format() },
            depth_attachment_format,
            rasterization,
            depth_stencil,
            color_blend,
            topology
        );
    }

    ComputePipeline* PipelineBuilder::build_compute_pipeline() const
    {
        if (!pipeline_layout_)
        {
            Log::error("Pipeline layout not created. Call build_pipeline_layout() first.");
            return nullptr;
        }

        if (shaders_.size() != 1 || shaders_[0]->stage() != ShaderStage::Compute)
        {
            Log::error("Compute pipeline requires exactly one compute shader");
            return nullptr;
        }

        auto* pipeline = create_compute_pipeline(
            api_, {
                .device = device_,
                .shader = shaders_[0],
                .layout = pipeline_layout_
            });

        if (!pipeline)
        {
            Log::error("Failed to create compute pipeline");
            return nullptr;
        }

        return pipeline;
    }

    const std::vector<DescriptorSetLayout*>& PipelineBuilder::get_descriptor_set_layouts() const
    {
        return descriptor_set_layouts_;
    }

    // ----------------------------------
    // ===== PipelineResourceBinder =====
    // ----------------------------------

    PipelineResourceBinder::PipelineResourceBinder(
        const std::vector<ShaderModule*>&  shaders,
        PipelineLayout*                    layout,
        const std::vector<DescriptorSet*>& descriptor_sets)
        : shaders_(shaders),
          layout_(layout),
          descriptor_sets_(descriptor_sets) { build_resource_maps(); }

    void PipelineResourceBinder::build_resource_maps()
    {
        for (const auto* shader : shaders_)
        {
            const auto& metadata = shader->meta_data();

            for (const auto& [name, pc] : metadata.push_constants)
            {
                PushConstantInfo info{ .pc = pc };
                for (const auto& member : pc.members) { info.members[member.name] = member; }
                push_constant_map_[name] = info;
            }

            for (const auto& [name, resource] : metadata.uniform_buffers)
            {
                resource_map_[name] = {
                    .set = resource.set,
                    .binding = resource.binding,
                    .type = DescriptorType::UniformBuffer,
                    .data_type = resource.data_type,
                    .size = resource.size
                };
            }

            for (const auto& [name, resource] : metadata.storage_buffers)
            {
                resource_map_[name] = {
                    .set = resource.set,
                    .binding = resource.binding,
                    .type = DescriptorType::StorageBuffer,
                    .data_type = resource.data_type,
                    .size = resource.size
                };
            }

            for (const auto& [name, resource] : metadata.sampled_images)
            {
                resource_map_[name] = {
                    .set = resource.set,
                    .binding = resource.binding,
                    .type = DescriptorType::CombinedImageSampler,
                    .data_type = resource.data_type,
                    .size = resource.size
                };
            }

            if (shader->stage() == ShaderStage::Vertex)
            {
                for (const auto& [name, input] : metadata.stage_inputs)
                    vertex_inputs_[name] = input;
            }
        }
    }

    template<typename T>
    bool PipelineResourceBinder::set_push_constant(CommandBuffer* cmd, const std::string& name, const T& value)
    {
        for (const auto& [pc, members] : push_constant_map_ | std::views::values)
        {
            if (members.contains(name))
            {
                const auto& member = members.at(name);

                if (sizeof(T) != member.size)
                {
                    Log::error("Push constant '{}' size mismatch: expected {}, got {}", name, member.size, sizeof(T));
                    return false;
                }

                cmd->push_constants(layout_, pc.stage, member.offset, sizeof(T), &value);
                return true;
            }
        }

        Log::error("Push constant '{}' not found in shader metadata. Available blocks: {}", name, push_constant_map_.size());
        for (const auto& [pc_name, pc_info] : push_constant_map_)
        {
            Log::error("  Block '{}': {} members", pc_name, pc_info.members.size());
            for (const auto& member_name : pc_info.members | std::views::keys)
                Log::error("    - {}", member_name);
        }

        return false;
    }

    template bool PipelineResourceBinder::set_push_constant<float>(CommandBuffer*, const std::string&, const float&);
    template bool PipelineResourceBinder::set_push_constant<int>(CommandBuffer*, const std::string&, const int&);
    template bool PipelineResourceBinder::set_push_constant<std::uint32_t>(CommandBuffer*, const std::string&, const std::uint32_t&);

    bool PipelineResourceBinder::bind_vertex_buffer(
        CommandBuffer*     cmd,
        const std::string& attribute_name,
        Buffer*            buffer) const
    {
        if (!vertex_inputs_.contains(attribute_name))
        {
            Log::error("Vertex attribute '{}' not found in shader", attribute_name);
            return false;
        }

        cmd->bind_vertex_buffer(buffer, 0, 0);
        return true;
    }

    bool PipelineResourceBinder::update_uniform_buffer(
        const std::string& name,
        Buffer*            buffer,
        const std::uint32_t     offset,
        std::uint32_t           range) const
    {
        if (!resource_map_.contains(name))
        {
            Log::error("Uniform buffer '{}' not found in shader", name);
            return false;
        }

        const auto& info = resource_map_.at(name);
        if (info.type != DescriptorType::UniformBuffer)
        {
            Log::error("Resource '{}' is not a uniform buffer", name);
            return false;
        }

        if (info.set >= descriptor_sets_.size())
        {
            Log::error("Descriptor set {} not allocated for resource '{}'", info.set, name);
            return false;
        }

        if (range == 0) range = static_cast<std::uint32_t>(buffer->size() - offset);

        descriptor_sets_[info.set]->update({
            DescriptorWrite{
                .binding = info.binding,
                .array_element = 0,
                .type = DescriptorType::UniformBuffer,
                .info = UniformBuffer{ buffer, offset, range }
            }
        });

        return true;
    }

    bool PipelineResourceBinder::update_storage_buffer(
        const std::string& name,
        Buffer*            buffer,
        const std::uint32_t     offset,
        std::uint32_t           range) const
    {
        if (!resource_map_.contains(name))
        {
            Log::error("Storage buffer '{}' not found in shader", name);
            return false;
        }

        const auto& info = resource_map_.at(name);
        if (info.type != DescriptorType::StorageBuffer)
        {
            Log::error("Resource '{}' is not a storage buffer", name);
            return false;
        }

        if (info.set >= descriptor_sets_.size())
        {
            Log::error("Descriptor set {} not allocated for resource '{}'", info.set, name);
            return false;
        }

        if (range == 0) { range = static_cast<std::uint32_t>(buffer->size() - offset); }

        descriptor_sets_[info.set]->update({
            DescriptorWrite{
                .binding = info.binding,
                .array_element = 0,
                .type = DescriptorType::StorageBuffer,
                .info = StorageBuffer{ buffer, offset, range }
            }
        });

        return true;
    }

    bool PipelineResourceBinder::update_sampler(const std::string& name, Texture* texture, Sampler* sampler) const
    {
        if (!resource_map_.contains(name))
        {
            Log::error("Sampler '{}' not found in shader", name);
            return false;
        }

        const auto& info = resource_map_.at(name);
        if (info.type != DescriptorType::CombinedImageSampler)
        {
            Log::error("Resource '{}' is not a combined image sampler", name);
            return false;
        }

        if (info.set >= descriptor_sets_.size())
        {
            Log::error("Descriptor set {} not allocated for resource '{}'", info.set, name);
            return false;
        }

        descriptor_sets_[info.set]->update({
            DescriptorWrite{
                .binding = info.binding,
                .array_element = 0,
                .type = DescriptorType::CombinedImageSampler,
                .info = CombinedImageSampler{ sampler, texture }
            }
        });

        return true;
    }

    void PipelineResourceBinder::bind_descriptor_sets(CommandBuffer* cmd) const
    {
        if (!descriptor_sets_.empty()) cmd->bind_descriptor_sets(layout_, descriptor_sets_, 0);
    }

    std::uint32_t PipelineResourceBinder::get_vertex_stride() const
    {
        std::uint32_t stride = 0;
        for (const auto& input : vertex_inputs_ | std::views::values) { stride += input.size; }
        return stride;
    }

    std::vector<VertexInputAttribute> PipelineResourceBinder::get_vertex_attributes() const
    {
        std::vector<VertexInputAttribute> attributes;
        std::uint32_t                          offset = 0;

        for (const auto& input : vertex_inputs_ | std::views::values)
        {
            attributes.push_back({
                .location = input.location,
                .binding = 0,
                .offset = offset
            });

            offset += input.size;
        }

        return attributes;
    }

    std::vector<VertexInputBinding> PipelineResourceBinder::get_vertex_bindings() const
    {
        if (vertex_inputs_.empty()) return {};
        return { { .binding = 0, .stride = get_vertex_stride(), .per_instance = false } };
    }


    size_t PipelineResourceBinder::get_type_size(const ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float: return 4;
            case ShaderDataType::Vec2: return 8;
            case ShaderDataType::Vec3: return 12;
            case ShaderDataType::Vec4: return 16;
            case ShaderDataType::Int: return 4;
            case ShaderDataType::IVec2: return 8;
            case ShaderDataType::IVec3: return 12;
            case ShaderDataType::IVec4: return 16;
            case ShaderDataType::Uint: return 4;
            case ShaderDataType::UVec2: return 8;
            case ShaderDataType::UVec3: return 12;
            case ShaderDataType::UVec4: return 16;
            case ShaderDataType::Mat2: return 16;
            case ShaderDataType::Mat3: return 36;
            case ShaderDataType::Mat4: return 64;
            default: return 0;
        }
    }
}