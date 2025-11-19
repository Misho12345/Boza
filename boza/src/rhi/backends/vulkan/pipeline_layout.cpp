module boza.rhi.vulkan;

import :pipeline;
import :util;

namespace boza::rhi::vk
{
    bool PipelineLayout::init()
    {
        // Log::trace("Creating vulkan pipeline layout");

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

        if (!vk_check(
            vkCreatePipelineLayout(device->logical_device(), &layout_info, nullptr, &vk_pipeline_layout_),
            "Failed to create pipeline layout"))
            return false;

        return true;
    }

    void PipelineLayout::destroy()
    {
        // Log::trace("Destroying vulkan pipeline layout");

        const auto* device = reinterpret_cast<Device*>(desc.device);
        if (vk_pipeline_layout_)
        {
            vkDestroyPipelineLayout(device->logical_device(), vk_pipeline_layout_, nullptr);
            vk_pipeline_layout_ = nullptr;
        }
    }

    VkPipelineLayout PipelineLayout::vk_pipeline_layout() const { return vk_pipeline_layout_; }
}