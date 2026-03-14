module boza.rhi;

import boza.core;
import :pipeline_builder;

namespace boza::rhi
{
    static constexpr const char* descriptor_type_to_string(const DescriptorType type)
    {
        switch (type)
        {
            case DescriptorType::Sampler: return "Sampler";
            case DescriptorType::CombinedImageSampler: return "CombinedImageSampler";
            case DescriptorType::SampledImage: return "SampledImage";
            case DescriptorType::StorageImage: return "StorageImage";
            case DescriptorType::UniformTexelBuffer: return "UniformTexelBuffer";
            case DescriptorType::StorageTexelBuffer: return "StorageTexelBuffer";
            case DescriptorType::UniformBuffer: return "UniformBuffer";
            case DescriptorType::StorageBuffer: return "StorageBuffer";
            case DescriptorType::UniformBufferDynamic: return "UniformBufferDynamic";
            case DescriptorType::StorageBufferDynamic: return "StorageBufferDynamic";
            case DescriptorType::InputAttachment: return "InputAttachment";
        }

        std::unreachable();
    }


    PipelineBuilder::PipelineBuilder(const GraphicsApi api, Device* device, std::vector<ShaderModule*> shaders)
        : api_{ api },
          device_{ device },
          shaders_{ std::move(shaders) } {}

    bool PipelineBuilder::build_descriptor_set_layouts()
    {
        if (!device_)
        {
            Log::error("Cannot build descriptor set layouts: device is null");
            return false;
        }

        flat_map<std::uint32_t, std::vector<DescriptorBinding>> bindings_by_set;
        if (!merge_descriptor_bindings(bindings_by_set)) return false;

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
                const auto& bindings = bindings_by_set.at(set_idx);
                layout_bindings.reserve(bindings.size());

                for (const auto& [binding, type, stages, count] : bindings)
                    layout_bindings.emplace_back(binding, type, stages, count);
            }

            auto layout = create_descriptor_set_layout(
                api_, {
                    .device = device_,
                    .bindings = layout_bindings
                });

            if (!layout)
            {
                Log::error("Failed to create descriptor set layout for set {}", set_idx);
                return false;
            }

            descriptor_set_layouts_.push_back(std::move(layout));
        }

        // Log::trace("Created {} descriptor set layout(s)", descriptor_set_layouts_.size());
        return true;
    }

    std::unique_ptr<PipelineLayout> PipelineBuilder::build_pipeline_layout() const
    {
        const auto pipeline_layouts = get_descriptor_set_layouts();

        auto pipeline_layout = create_pipeline_layout(
            api_, {
                .device = device_,
                .shaders = shaders_,
                .set_layouts = pipeline_layouts
            });

        if (!pipeline_layout)
        {
            Log::error("Failed to create pipeline layout");
            return nullptr;
        }

        // Log::trace("Created pipeline layout with {} descriptor set(s)", descriptor_set_layouts_.size());
        return pipeline_layout;
    }

    std::unique_ptr<GraphicsPipeline> PipelineBuilder::build_graphics_pipeline(
        PipelineLayout*                   pipeline_layout,
        const std::vector<TextureFormat>& color_attachment_formats,
        const DepthFormat                 depth_attachment_format,
        const RasterizationState&         rasterization,
        const DepthStencilState&          depth_stencil,
        const ColorBlendState&            color_blend,
        const PrimitiveTopology           topology) const
    {
        if (!pipeline_layout)
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
            const auto&   metadata     = vertex_shader->meta_data();
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

        auto pipeline = create_graphics_pipeline(
            api_, {
                .device = device_,
                .shaders = shaders_,
                .layout = pipeline_layout,
                .topology = topology,
                .bindings = bindings,
                .attributes = attributes,
                .rasterization = rasterization,
                .depth_stencil = depth_stencil,
                .color_blend = adjusted_color_blend,
                .color_attachment_formats = color_attachment_formats,
                .depth_attachment_format = depth_attachment_format,
                .stencil_attachment_format = DepthFormat::None
            });

        if (!pipeline) return nullptr;

        // Log::trace("Created graphics pipeline");
        return pipeline;
    }

    std::unique_ptr<GraphicsPipeline> PipelineBuilder::build_graphics_pipeline(
        PipelineLayout*          pipeline_layout,
        const Swapchain*         swapchain,
        const DepthFormat        depth_attachment_format,
        const RasterizationState& rasterization,
        const DepthStencilState& depth_stencil,
        const ColorBlendState&   color_blend,
        const PrimitiveTopology  topology) const
    {
        if (!swapchain)
        {
            Log::error("Cannot build graphics pipeline: swapchain is null");
            return nullptr;
        }

        return build_graphics_pipeline(
            pipeline_layout,
            { swapchain->format() },
            depth_attachment_format,
            rasterization,
            depth_stencil,
            color_blend,
            topology
        );
    }

    std::unique_ptr<ComputePipeline> PipelineBuilder::build_compute_pipeline(PipelineLayout* pipeline_layout) const
    {
        if (!pipeline_layout)
        {
            Log::error("Pipeline layout not created. Call build_pipeline_layout() first.");
            return nullptr;
        }

        if (shaders_.size() != 1 || shaders_[0]->stage() != ShaderStage::Compute)
        {
            Log::error("Compute pipeline requires exactly one compute shader");
            return nullptr;
        }

        auto pipeline = create_compute_pipeline(
            api_, {
                .device = device_,
                .shader = shaders_[0],
                .layout = pipeline_layout
            });

        if (!pipeline)
        {
            Log::error("Failed to create compute pipeline");
            return nullptr;
        }

        return pipeline;
    }

    std::vector<DescriptorSetLayout*> PipelineBuilder::get_descriptor_set_layouts() const
    {
        std::vector<DescriptorSetLayout*> layouts;
        layouts.reserve(descriptor_set_layouts_.size());

        for (const auto& layout : descriptor_set_layouts_)
            layouts.push_back(layout.get());

        return layouts;
    }

    std::vector<std::unique_ptr<DescriptorSetLayout>> PipelineBuilder::take_descriptor_set_layouts()
    {
        return std::move(descriptor_set_layouts_);
    }

    bool PipelineBuilder::merge_descriptor_bindings(
        flat_map<std::uint32_t, std::vector<DescriptorBinding>>& bindings_by_set) const
    {
        bindings_by_set.clear();

        const auto merge_binding =
            [&](const ShaderModule::ShaderResource& resource,
                const DescriptorType descriptor_type,
                const Flags<ShaderStage> stage_flag,
                const char* resource_group,
                const std::string& shader_name) -> bool
        {
            if (resource.set == std::numeric_limits<std::uint32_t>::max() ||
                resource.binding == std::numeric_limits<std::uint32_t>::max())
            {
                Log::error(
                    "Shader '{}' has {} descriptor '{}' with invalid set/binding metadata",
                    shader_name,
                    resource_group,
                    resource.type_name
                );
                return false;
            }

            constexpr std::uint32_t descriptor_count = 1;

            auto& bindings = bindings_by_set[resource.set];

            auto it = std::ranges::find_if(bindings, [&](const DescriptorBinding& b)
            {
                return b.binding == resource.binding;
            });

            if (it != bindings.end())
            {
                if (it->type != descriptor_type || it->count != descriptor_count)
                {
                    Log::error(
                        "Descriptor binding mismatch at set {}, binding {} while merging shader '{}': existing {} x{}, new {} x{}",
                        resource.set,
                        resource.binding,
                        shader_name,
                        descriptor_type_to_string(it->type),
                        it->count,
                        descriptor_type_to_string(descriptor_type),
                        descriptor_count
                    );

                    return false;
                }

                it->stages |= stage_flag;
                return true;
            }

            bindings.emplace_back(
                resource.binding,
                descriptor_type,
                stage_flag,
                descriptor_count
            );

            return true;
        };

        const auto merge_resource_group =
            [&](const auto& resources,
                const DescriptorType descriptor_type,
                const char* resource_group,
                const std::string& shader_name,
                const Flags<ShaderStage> stage_flag) -> bool
        {
            for (const auto& resource : resources | std::views::values)
            {
                if (!merge_binding(resource, descriptor_type, stage_flag, resource_group, shader_name))
                    return false;
            }

            return true;
        };

        for (const auto* shader : shaders_)
        {
            if (!shader)
            {
                Log::error("Cannot merge descriptor bindings: encountered null shader module");
                return false;
            }

            const auto& metadata   = shader->meta_data();
            const auto  stage_flag = Flags(shader->stage());
            const auto  shader_name = shader->filename();

            if (!merge_resource_group(metadata.uniform_buffers, DescriptorType::UniformBuffer, "uniform_buffer", shader_name, stage_flag)) return false;
            if (!merge_resource_group(metadata.storage_buffers, DescriptorType::StorageBuffer, "storage_buffer", shader_name, stage_flag)) return false;
            if (!merge_resource_group(metadata.sampled_images, DescriptorType::CombinedImageSampler, "sampled_image", shader_name, stage_flag)) return false;
            if (!merge_resource_group(metadata.storage_images, DescriptorType::StorageImage, "storage_image", shader_name, stage_flag)) return false;
        }

        return true;
    }
}
