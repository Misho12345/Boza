module boza.rhi.vulkan;

import :pipeline;
import :util;

namespace boza::rhi::vk
{
    bool GraphicsPipeline::init()
    {
        // Log::trace("Creating vulkan graphics pipeline");

        const auto* device = reinterpret_cast<Device*>(desc.device);
        const auto* layout = reinterpret_cast<PipelineLayout*>(desc.layout);

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        shader_stages.reserve(desc.shaders.size());

        for (const auto* shader : desc.shaders)
        {
            const auto*                     vk_shader = reinterpret_cast<const ShaderModule*>(shader);
            VkPipelineShaderStageCreateInfo stage_info
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = to_vk(shader->stage()),
                .module = vk_shader->vk_shader_module(),
                .pName = "main",
                .pSpecializationInfo = nullptr
            };
            shader_stages.push_back(stage_info);
        }

        std::vector<VkVertexInputBindingDescription> binding_descriptions;
        binding_descriptions.reserve(desc.bindings.size());

        for (const auto& [binding, stride, per_instance] : desc.bindings)
        {
            VkVertexInputBindingDescription binding_desc
            {
                .binding = binding,
                .stride = stride,
                .inputRate = per_instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX
            };
            binding_descriptions.push_back(binding_desc);
        }

        std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
        attribute_descriptions.reserve(desc.attributes.size());

        // Build a map from location to shader input for quick lookup
        std::unordered_map<uint32_t, const rhi::ShaderModule::ShaderResource*> location_to_input;
        for (const auto* shader : desc.shaders)
        {
            if (shader->stage() == ShaderStage::Vertex)
            {
                for (const auto& input : shader->meta_data().stage_inputs | std::views::values) location_to_input[input.
                    location] = &input;
            }
        }

        for (const auto& [location, binding, offset] : desc.attributes)
        {
            VkFormat format = VK_FORMAT_R32G32B32_SFLOAT;

            if (const auto it = location_to_input.find(location); it != location_to_input.end())
            {
                format = to_vk(it->second->data_type);
            }
            else Log::warn("Could not find shader input for location {}, using default format", location);

            attribute_descriptions.emplace_back(
                location,
                binding,
                format,
                offset
            );
        }

        const VkPipelineVertexInputStateCreateInfo vertex_input_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = static_cast<uint32_t>(binding_descriptions.size()),
            .pVertexBindingDescriptions = binding_descriptions.empty() ? nullptr : binding_descriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size()),
            .pVertexAttributeDescriptions = attribute_descriptions.empty() ? nullptr : attribute_descriptions.data()
        };

        const VkPipelineInputAssemblyStateCreateInfo input_assembly
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .topology = to_vk(desc.topology),
            .primitiveRestartEnable = false
        };

        constexpr VkPipelineViewportStateCreateInfo viewport_state
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .viewportCount = 1,
            .pViewports = nullptr,
            .scissorCount = 1,
            .pScissors = nullptr
        };

        const VkPipelineRasterizationStateCreateInfo rasterizer
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthClampEnable = desc.rasterization.depth_clamp_enable,
            .rasterizerDiscardEnable = false,
            .polygonMode = to_vk(desc.rasterization.polygon_mode),
            .cullMode = to_vk(desc.rasterization.cull_mode),
            .frontFace = to_vk(desc.rasterization.front_face),
            .depthBiasEnable = desc.rasterization.depth_bias_enable,
            .depthBiasConstantFactor = desc.rasterization.depth_bias_constant,
            .depthBiasClamp = desc.rasterization.depth_bias_clamp,
            .depthBiasSlopeFactor = desc.rasterization.depth_bias_slope,
            .lineWidth = desc.rasterization.line_width
        };

        constexpr VkPipelineMultisampleStateCreateInfo multisampling
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = false,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = false,
            .alphaToOneEnable = false
        };

        const VkPipelineDepthStencilStateCreateInfo depth_stencil
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = desc.depth_stencil.depth_test_enable,
            .depthWriteEnable = desc.depth_stencil.depth_write_enable,
            .depthCompareOp = to_vk(desc.depth_stencil.depth_compare_op),
            .depthBoundsTestEnable = desc.depth_stencil.depth_bounds_test_enable,
            .stencilTestEnable = desc.depth_stencil.stencil_test_enable,
            .front = {},
            .back = {},
            .minDepthBounds = desc.depth_stencil.min_depth_bounds,
            .maxDepthBounds = desc.depth_stencil.max_depth_bounds
        };

        std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
        color_blend_attachments.reserve(desc.color_blend.attachments.size());

        for (const auto& [blend_enable,
                 src_color_blend_factor, dst_color_blend_factor, color_blend_op,
                 src_alpha_blend_factor, dst_alpha_blend_factor, alpha_blend_op] : desc.color_blend.attachments)
        {
            VkPipelineColorBlendAttachmentState blend_attachment
            {
                .blendEnable = blend_enable,
                .srcColorBlendFactor = to_vk(src_color_blend_factor),
                .dstColorBlendFactor = to_vk(dst_color_blend_factor),
                .colorBlendOp = to_vk(color_blend_op),
                .srcAlphaBlendFactor = to_vk(src_alpha_blend_factor),
                .dstAlphaBlendFactor = to_vk(dst_alpha_blend_factor),
                .alphaBlendOp = to_vk(alpha_blend_op),
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
            };
            color_blend_attachments.push_back(blend_attachment);
        }

        const VkPipelineColorBlendStateCreateInfo color_blending
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = desc.color_blend.logic_op_enable,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = static_cast<uint32_t>(color_blend_attachments.size()),
            .pAttachments = color_blend_attachments.empty() ? nullptr : color_blend_attachments.data(),
            .blendConstants = {
                desc.color_blend.blend_constants[0],
                desc.color_blend.blend_constants[1],
                desc.color_blend.blend_constants[2],
                desc.color_blend.blend_constants[3]
            }
        };

        const std::vector dynamic_states
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        const VkPipelineDynamicStateCreateInfo dynamic_state
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
            .pDynamicStates = dynamic_states.data()
        };

        std::vector<VkFormat> color_formats;
        color_formats.reserve(desc.color_attachment_formats.size());
        for (const auto format : desc.color_attachment_formats)
        {
            color_formats.push_back(static_cast<VkFormat>(format));
        }

        VkPipelineRenderingCreateInfo rendering_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = static_cast<uint32_t>(color_formats.size()),
            .pColorAttachmentFormats = color_formats.empty() ? nullptr : color_formats.data(),
            .depthAttachmentFormat = static_cast<VkFormat>(desc.depth_attachment_format),
            .stencilAttachmentFormat = static_cast<VkFormat>(desc.stencil_attachment_format)
        };

        const VkGraphicsPipelineCreateInfo pipeline_info
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &rendering_info,
            .flags = 0,
            .stageCount = static_cast<uint32_t>(shader_stages.size()),
            .pStages = shader_stages.data(),
            .pVertexInputState = &vertex_input_info,
            .pInputAssemblyState = &input_assembly,
            .pTessellationState = nullptr,
            .pViewportState = &viewport_state,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depth_stencil,
            .pColorBlendState = &color_blending,
            .pDynamicState = &dynamic_state,
            .layout = layout->vk_pipeline_layout(),
            .renderPass = nullptr,
            // Using dynamic rendering
            .subpass = 0,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1
        };

        if (!vk_check(
            vkCreateGraphicsPipelines(device->logical_device(), nullptr, 1, &pipeline_info, nullptr, &vk_pipeline_),
            "Failed to create graphics pipeline"))
            return false;

        return true;
    }

    void GraphicsPipeline::destroy()
    {
        // Log::trace("Destroying vulkan graphics pipeline");

        const auto* device = reinterpret_cast<Device*>(desc.device);
        if (vk_pipeline_)
        {
            vkDestroyPipeline(device->logical_device(), vk_pipeline_, nullptr);
            vk_pipeline_ = nullptr;
        }
    }

    VkPipeline GraphicsPipeline::vk_pipeline() const { return vk_pipeline_; }
}
