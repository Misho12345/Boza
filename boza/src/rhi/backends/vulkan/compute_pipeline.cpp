module boza.rhi.vulkan;

import :pipeline;
import :util;

namespace boza::rhi::vk
{
    bool ComputePipeline::init()
    {
        // Log::trace("Creating vulkan compute pipeline");

        const auto* device = reinterpret_cast<Device*>(desc_.device);
        const auto* layout = reinterpret_cast<PipelineLayout*>(desc_.layout);
        const auto* shader = reinterpret_cast<const ShaderModule*>(desc_.shader);

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

        if (!vk_check(
            vkCreateComputePipelines(device->logical_device(), nullptr, 1, &pipeline_info, nullptr, &vk_pipeline_),
            "Failed to create compute pipeline"))
            return false;

        return true;
    }

    void ComputePipeline::destroy()
    {
        // Log::trace("Destroying vulkan compute pipeline");

        const auto* device = reinterpret_cast<Device*>(desc_.device);
        if (vk_pipeline_)
        {
            vkDestroyPipeline(device->logical_device(), vk_pipeline_, nullptr);
            vk_pipeline_ = nullptr;
        }
    }

    VkPipeline ComputePipeline::vk_pipeline() const { return vk_pipeline_; }
}