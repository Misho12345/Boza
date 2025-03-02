#include "Pipeline.hpp"

#include "Device.hpp"
#include "Logger.hpp"
#include "Shader.hpp"
#include "Swapchain.hpp"

namespace boza
{
    bool Pipeline::create()
    {
        Logger::trace("Creating graphics pipeline");
        return instance().create_pipeline();
    }

    void Pipeline::destroy()
    {
        const auto& inst = instance();

        Shader::destroy_shader_module(inst.vertex_module);
        Shader::destroy_shader_module(inst.fragment_module);

        const auto& device = Device::get_device();
        if (device == nullptr) return;

        vkDestroyPipeline(device, inst.pipeline, nullptr);
        vkDestroyPipelineLayout(device, inst.pipeline_layout, nullptr);
    }

    VkPipeline& Pipeline::get_pipeline() { return instance().pipeline; }

    bool Pipeline::create_pipeline()
    {
        vertex_module = Shader::create_shader_module("default.vert");
        if (vertex_module == VK_NULL_HANDLE)
        {
            Logger::critical("Failed to create vertex shader module");
            return false;
        }

        fragment_module = Shader::create_shader_module("default.frag");
        if (fragment_module == VK_NULL_HANDLE)
        {
            Logger::critical("Failed to create fragment shader module");
            return false;
        }

        const VkPipelineShaderStageCreateInfo shader_stages[]
        {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertex_module,
                .pName = "main",
                .pSpecializationInfo = nullptr
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragment_module,
                .pName = "main",
                .pSpecializationInfo = nullptr
            }
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_info
        {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            nullptr,
            {},
            0, nullptr,
            0, nullptr
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
            .maxDepth = 0.0f
        };

        VkRect2D scissor
        {
            .offset = {0, 0},
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
            .polygonMode = VK_POLYGON_MODE_FILL,
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

        if (!create_pipeline_layout()) return false;

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
            .pDynamicState = nullptr,
            .layout = pipeline_layout,
            .renderPass = VK_NULL_HANDLE,
            .subpass = {},
            .basePipelineHandle = nullptr,
            .basePipelineIndex = {}
        };

        VK_CHECK(vkCreateGraphicsPipelines(Device::get_device(), nullptr, 1, &pipeline_info, nullptr, &pipeline),
        {
            LOG_VK_RESULT("Failed to create graphics pipeline");
            return false;
        });

        return true;
    }

    bool Pipeline::create_pipeline_layout()
    {
        constexpr VkPipelineLayoutCreateInfo layout_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .setLayoutCount = 0,
            .pSetLayouts = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        VK_CHECK(vkCreatePipelineLayout(Device::get_device(), &layout_info, nullptr, &pipeline_layout),
        {
            LOG_VK_RESULT("Failed to create pipeline layout");
            return false;
        });

        return true;
    }
}
