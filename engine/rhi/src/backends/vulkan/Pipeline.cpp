#include "Pipeline.hpp"

#include <ranges>

#include "Device.hpp"
#include "ShaderModule.hpp"
#include "Descriptor.hpp"
#include "boza/core/Logger.hpp"

namespace boza::rhi::vk
{
    namespace
    {
        VkShaderStageFlagBits to_vk(const ShaderStage stage)
        {
            switch (stage)
            {
                case ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
                case ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
                case ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
                case ShaderStage::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
                case ShaderStage::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                case ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
                default: return VK_SHADER_STAGE_ALL;
            }
        }

        VkPrimitiveTopology to_vk(const PrimitiveTopology topology)
        {
            switch (topology)
            {
                case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                case PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                case PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
                default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            }
        }

        VkPolygonMode to_vk(const PolygonMode mode)
        {
            switch (mode)
            {
                case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
                case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
                case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
                default: return VK_POLYGON_MODE_FILL;
            }
        }

        VkCullModeFlags to_vk(const CullMode mode)
        {
            switch (mode)
            {
                case CullMode::None: return VK_CULL_MODE_NONE;
                case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
                case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
                case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
                default: return VK_CULL_MODE_BACK_BIT;
            }
        }

        VkFrontFace to_vk(const FrontFace face)
        {
            switch (face)
            {
                case FrontFace::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
                case FrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
                default: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            }
        }

        VkCompareOp to_vk(const CompareOp op)
        {
            switch (op)
            {
                case CompareOp::Never: return VK_COMPARE_OP_NEVER;
                case CompareOp::Less: return VK_COMPARE_OP_LESS;
                case CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
                case CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
                case CompareOp::Greater: return VK_COMPARE_OP_GREATER;
                case CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
                case CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
                case CompareOp::Always: return VK_COMPARE_OP_ALWAYS;
                default: return VK_COMPARE_OP_LESS;
            }
        }

        VkBlendFactor to_vk(const BlendFactor factor)
        {
            switch (factor)
            {
                case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
                case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
                case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
                case BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                case BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
                case BlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
                case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
                case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
                case BlendFactor::ConstantColor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
                case BlendFactor::OneMinusConstantColor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
                case BlendFactor::ConstantAlpha: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
                case BlendFactor::OneMinusConstantAlpha: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
                case BlendFactor::SrcAlphaSaturate: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
                default: return VK_BLEND_FACTOR_ZERO;
            }
        }

        VkBlendOp to_vk(const BlendOp op)
        {
            switch (op)
            {
                case BlendOp::Add: return VK_BLEND_OP_ADD;
                case BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
                case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
                case BlendOp::Min: return VK_BLEND_OP_MIN;
                case BlendOp::Max: return VK_BLEND_OP_MAX;
                default: return VK_BLEND_OP_ADD;
            }
        }

        VkFormat to_vk(const ShaderDataType type)
        {
            switch (type)
            {
                case ShaderDataType::Float: return VK_FORMAT_R32_SFLOAT;
                case ShaderDataType::Vec2: return VK_FORMAT_R32G32_SFLOAT;
                case ShaderDataType::Vec3: return VK_FORMAT_R32G32B32_SFLOAT;
                case ShaderDataType::Vec4: return VK_FORMAT_R32G32B32A32_SFLOAT;
                case ShaderDataType::Int: return VK_FORMAT_R32_SINT;
                case ShaderDataType::IVec2: return VK_FORMAT_R32G32_SINT;
                case ShaderDataType::IVec3: return VK_FORMAT_R32G32B32_SINT;
                case ShaderDataType::IVec4: return VK_FORMAT_R32G32B32A32_SINT;
                case ShaderDataType::Uint: return VK_FORMAT_R32_UINT;
                case ShaderDataType::UVec2: return VK_FORMAT_R32G32_UINT;
                case ShaderDataType::UVec3: return VK_FORMAT_R32G32B32_UINT;
                case ShaderDataType::UVec4: return VK_FORMAT_R32G32B32A32_UINT;
                case ShaderDataType::Double: return VK_FORMAT_R64_SFLOAT;
                default: return VK_FORMAT_R32G32B32_SFLOAT;
            }
        }
    }

    /// ---------------------------
    /// ===== Pipeline Layout =====
    /// ---------------------------

    bool PipelineLayout::init()
    {
        // Logger::trace("Creating vulkan pipeline layout");

        const auto* device = reinterpret_cast<Device*>(desc.device);

        for (const auto* shader : desc.shaders)
        {
            const auto& meta_data = shader->meta_data();
            for (const auto& [name, pc] : meta_data.push_constants)
            {
                push_constants_[name] = pc;
            }
        }

        std::vector<VkPushConstantRange> push_constant_ranges;
        push_constant_ranges.reserve(push_constants_.size());

        for (const auto& pc : push_constants_ | std::views::values)
        {
            VkPushConstantRange range{};
            range.stageFlags = to_vk(pc.stage);
            range.offset = pc.offset;
            range.size = pc.size;
            push_constant_ranges.push_back(range);
        }

        std::vector<VkDescriptorSetLayout> vk_set_layouts;
        vk_set_layouts.reserve(desc.set_layouts.size());
        for (const auto* layout : desc.set_layouts)
        {
            vk_set_layouts.push_back(reinterpret_cast<const DescriptorSetLayout*>(layout)->vk_descriptor_set_layout());
        }

        const VkPipelineLayoutCreateInfo layout_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = static_cast<uint32_t>(vk_set_layouts.size()),
            .pSetLayouts = vk_set_layouts.empty() ? nullptr : vk_set_layouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
            .pPushConstantRanges = push_constant_ranges.empty() ? nullptr : push_constant_ranges.data()
        };

        VK_CHECK(vkCreatePipelineLayout(device->logical_device(), &layout_info, nullptr, &vk_pipeline_layout_),
        {
            LOG_VK_ERROR("Failed to create pipeline layout");
            return false;
        });

        return true;
    }

    void PipelineLayout::destroy()
    {
        // Logger::trace("Destroying vulkan pipeline layout");

        const auto* device = reinterpret_cast<Device*>(desc.device);
        if (vk_pipeline_layout_)
        {
            vkDestroyPipelineLayout(device->logical_device(), vk_pipeline_layout_, nullptr);
            vk_pipeline_layout_ = nullptr;
        }
    }

    VkPipelineLayout PipelineLayout::vk_pipeline_layout() const { return vk_pipeline_layout_; }

    /// -----------------------------
    /// ===== Graphics Pipeline =====
    /// -----------------------------

    bool GraphicsPipeline::init()
    {
        // Logger::trace("Creating vulkan graphics pipeline");

        const auto* device = reinterpret_cast<Device*>(desc.device);
        const auto* layout = reinterpret_cast<PipelineLayout*>(desc.layout);

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
        shader_stages.reserve(desc.shaders.size());

        for (const auto* shader : desc.shaders)
        {
            const auto* vk_shader = reinterpret_cast<const ShaderModule*>(shader);
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
                for (const auto& input : shader->meta_data().stage_inputs | std::views::values)
                {
                    location_to_input[input.location] = &input;
                }
            }
        }

        for (const auto& [location, binding, offset] : desc.attributes)
        {
            VkFormat format = VK_FORMAT_R32G32B32_SFLOAT;

            if (const auto it = location_to_input.find(location); it != location_to_input.end())
            {
                format = to_vk(it->second->data_type);
            }
            else
            {
                Logger::warn("Could not find shader input for location {}, using default format", location);
            }

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
            .primitiveRestartEnable = VK_FALSE
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
            .depthClampEnable = desc.rasterization.depth_clamp_enable ? VK_TRUE : VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = to_vk(desc.rasterization.polygon_mode),
            .cullMode = to_vk(desc.rasterization.cull_mode),
            .frontFace = to_vk(desc.rasterization.front_face),
            .depthBiasEnable = desc.rasterization.depth_bias_enable ? VK_TRUE : VK_FALSE,
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
            .sampleShadingEnable = VK_FALSE,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE
        };

        const VkPipelineDepthStencilStateCreateInfo depth_stencil
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .depthTestEnable = desc.depth_stencil.depth_test_enable ? VK_TRUE : VK_FALSE,
            .depthWriteEnable = desc.depth_stencil.depth_write_enable ? VK_TRUE : VK_FALSE,
            .depthCompareOp = to_vk(desc.depth_stencil.depth_compare_op),
            .depthBoundsTestEnable = desc.depth_stencil.depth_bounds_test_enable ? VK_TRUE : VK_FALSE,
            .stencilTestEnable = desc.depth_stencil.stencil_test_enable ? VK_TRUE : VK_FALSE,
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
                .blendEnable = blend_enable ? VK_TRUE : VK_FALSE,
                .srcColorBlendFactor = to_vk(src_color_blend_factor),
                .dstColorBlendFactor = to_vk(dst_color_blend_factor),
                .colorBlendOp = to_vk(color_blend_op),
                .srcAlphaBlendFactor = to_vk(src_alpha_blend_factor),
                .dstAlphaBlendFactor = to_vk(dst_alpha_blend_factor),
                .alphaBlendOp = to_vk(alpha_blend_op),
                .colorWriteMask = (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT).value()
            };
            color_blend_attachments.push_back(blend_attachment);
        }

        const VkPipelineColorBlendStateCreateInfo color_blending
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .logicOpEnable = desc.color_blend.logic_op_enable ? VK_TRUE : VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = static_cast<uint32_t>(color_blend_attachments.size()),
            .pAttachments = color_blend_attachments.empty() ? nullptr : color_blend_attachments.data(),
            .blendConstants = { desc.color_blend.blend_constants[0], desc.color_blend.blend_constants[1],
                               desc.color_blend.blend_constants[2], desc.color_blend.blend_constants[3] }
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
            .renderPass = nullptr, // Using dynamic rendering
            .subpass = 0,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1
        };

        VK_CHECK(vkCreateGraphicsPipelines(device->logical_device(), nullptr, 1, &pipeline_info, nullptr, &vk_pipeline_),
        {
            LOG_VK_ERROR("Failed to create graphics pipeline");
            return false;
        });

        return true;
    }

    void GraphicsPipeline::destroy()
    {
        // Logger::trace("Destroying vulkan graphics pipeline");

        const auto* device = reinterpret_cast<Device*>(desc.device);
        if (vk_pipeline_)
        {
            vkDestroyPipeline(device->logical_device(), vk_pipeline_, nullptr);
            vk_pipeline_ = nullptr;
        }
    }

    VkPipeline GraphicsPipeline::vk_pipeline() const { return vk_pipeline_; }

    /// ----------------------------
    /// ===== Compute Pipeline =====
    /// ----------------------------

    bool ComputePipeline::init()
    {
        // Logger::trace("Creating vulkan compute pipeline");

        const auto* device = reinterpret_cast<Device*>(desc.device);
        const auto* layout = reinterpret_cast<PipelineLayout*>(desc.layout);
        const auto* shader = reinterpret_cast<const ShaderModule*>(desc.shader);

        const VkPipelineShaderStageCreateInfo shader_stage
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader->vk_shader_module(),
            .pName = "main",
            .pSpecializationInfo = nullptr
        };

        const VkComputePipelineCreateInfo pipeline_info
        {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = shader_stage,
            .layout = layout->vk_pipeline_layout(),
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1
        };

        VK_CHECK(vkCreateComputePipelines(device->logical_device(), nullptr, 1, &pipeline_info, nullptr, &vk_pipeline_),
        {
            LOG_VK_ERROR("Failed to create compute pipeline");
            return false;
        });

        return true;
    }

    void ComputePipeline::destroy()
    {
        // Logger::trace("Destroying vulkan compute pipeline");

        const auto* device = reinterpret_cast<Device*>(desc.device);
        if (vk_pipeline_)
        {
            vkDestroyPipeline(device->logical_device(), vk_pipeline_, nullptr);
            vk_pipeline_ = nullptr;
        }
    }

    VkPipeline ComputePipeline::vk_pipeline() const { return vk_pipeline_; }
}