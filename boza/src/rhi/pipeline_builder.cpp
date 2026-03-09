module boza.rhi;

import boza.core;
import :pipeline_builder;

namespace boza::rhi
{
     /// ----------------------------
    /// ===== Pipeline Builder =====
    /// ----------------------------

    PipelineBuilder::PipelineBuilder(const GraphicsApi api, Device* device, const std::vector<ShaderModule*>& shaders)
        : api_{ api },
          device_{ device },
          shaders_{ std::move(shaders) } {}

    bool PipelineBuilder::build_descriptor_set_layouts()
    {
        auto bindings_by_set = merge_descriptor_bindings();

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

    flat_map<std::uint32_t, std::vector<PipelineBuilder::DescriptorBinding>>
    PipelineBuilder::merge_descriptor_bindings() const
    {
        flat_map<std::uint32_t, std::vector<DescriptorBinding>> bindings_by_set;

        // TODO: remove duplication for each bindings for each resource type
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

                if (it != bindings.end()) it->stages |= stage_flag;
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

                if (it != bindings.end()) it->stages |= stage_flag;
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

                if (it != bindings.end()) it->stages |= stage_flag;
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
}
