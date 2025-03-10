#include "Pipeline.hpp"

#include "Device.hpp"
#include "Logger.hpp"
#include "Swapchain.hpp"

namespace boza
{
    Pipeline::Pipeline(const PipelineCreateInfo& create_info)
    {
        if (!create_pipeline(create_info)) Logger::critical("failed to create pipeline");
    }

    Pipeline::~Pipeline()
    {
        const auto& device = Device::get_device();

        if (device == nullptr) return;

        if (pipeline != nullptr) vkDestroyPipeline(device, pipeline, nullptr);
        if (layout != nullptr) vkDestroyPipelineLayout(device, layout, nullptr);
    }

    Pipeline::Pipeline(Pipeline&& other) noexcept
    {
        pipeline = std::exchange(other.pipeline, nullptr);
        layout = std::exchange(other.layout, nullptr);
    }

    Pipeline& Pipeline::operator=(Pipeline&& other) noexcept
    {
        if (this != &other)
        {
            pipeline = std::exchange(other.pipeline, nullptr);
            layout = std::exchange(other.layout, nullptr);
        }

        return *this;
    }


    VkPipeline&       Pipeline::get_pipeline() { return pipeline; }
    VkPipelineLayout& Pipeline::get_layout() { return layout; }


    bool Pipeline::create_pipeline(const PipelineCreateInfo& create_info)
    {
        const VkPipelineShaderStageCreateInfo shader_stages[]
        {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = create_info.vertex_shader,
                .pName = "main",
                .pSpecializationInfo = nullptr
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = create_info.fragment_shader,
                .pName = "main",
                .pSpecializationInfo = nullptr
            }
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_info
        {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            nullptr,
            {},
            1,
            &create_info.binding_description,
            static_cast<uint32_t>(create_info.attribute_descriptions.size()),
            create_info.attribute_descriptions.data(),
        };

        VkPipelineInputAssemblyStateCreateInfo input_assembly_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        VkViewport viewport
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(Swapchain::get_extent().width),
            .height = static_cast<float>(Swapchain::get_extent().height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        VkRect2D scissor
        {
            .offset = { 0, 0 },
            .extent = Swapchain::get_extent()
        };

        VkPipelineViewportStateCreateInfo viewport_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor
        };

        VkPipelineRasterizationStateCreateInfo rasterization_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = create_info.polygon_mode,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f
        };

        VkPipelineMultisampleStateCreateInfo multisample_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE
        };

        VkPipelineColorBlendAttachmentState color_blend_attachment
        {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT
        };

        VkPipelineColorBlendStateCreateInfo color_blend_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &color_blend_attachment,
            .blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
        };

        if (!create_pipeline_layout(create_info)) return false;

        VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &Swapchain::get_format(),
            .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
        };

        VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamic_state_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext = nullptr,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamic_states
        };

        VkGraphicsPipelineCreateInfo pipeline_info
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &pipeline_rendering_create_info,
            .flags = {},
            .stageCount = std::size(shader_stages),
            .pStages = shader_stages,
            .pVertexInputState = &vertex_input_info,
            .pInputAssemblyState = &input_assembly_info,
            .pTessellationState = nullptr,
            .pViewportState = &viewport_info,
            .pRasterizationState = &rasterization_info,
            .pMultisampleState = &multisample_info,
            .pDepthStencilState = nullptr,
            .pColorBlendState = &color_blend_info,
            .pDynamicState = &dynamic_state_info,
            .layout = layout,
            .renderPass = nullptr,
            .subpass = {},
            .basePipelineHandle = nullptr,
            .basePipelineIndex = 0
        };

        VK_CHECK(vkCreateGraphicsPipelines(Device::get_device(), nullptr, 1, &pipeline_info, nullptr, &pipeline),
        {
            LOG_VK_ERROR("Failed to create graphics pipeline");
            return false;
        });

        return true;
    }

    bool Pipeline::create_pipeline_layout(const PipelineCreateInfo& create_info)
    {
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        descriptor_set_layouts.reserve(create_info.descriptor_set_layouts.size());
        for (const auto& set_layout : create_info.descriptor_set_layouts)
            descriptor_set_layouts.push_back(set_layout);

        const VkPipelineLayoutCreateInfo layout_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size()),
            .pSetLayouts = descriptor_set_layouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(create_info.push_constant_ranges.size()),
            .pPushConstantRanges = create_info.push_constant_ranges.data(),
        };

        VK_CHECK(vkCreatePipelineLayout(Device::get_device(), &layout_info, nullptr, &layout),
        {
            LOG_VK_ERROR("Failed to create pipeline layout");
            return false;
        });

        return true;
    }
}
