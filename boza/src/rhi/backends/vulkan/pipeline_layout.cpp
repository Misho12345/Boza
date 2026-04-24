module boza.rhi.vulkan;

import :pipeline;
import :util;

namespace boza::rhi::vk
{
    bool PipelineLayout::init()
    {
        // Log::trace("Creating vulkan pipeline layout");

        const auto* device = reinterpret_cast<Device*>(desc_.device);
        bool has_push_constant_conflict = false;

        for (const auto* shader : desc_.shaders)
        {
            const auto& meta_data = shader->meta_data();
            for (const auto& [name, pc] : meta_data.push_constants)
            {
                const auto [it, inserted] = push_constants_.try_emplace(name, pc);
                if (inserted) continue;

                auto& existing = it->second;

                if (existing.offset != pc.offset || existing.size != pc.size)
                {
                    Log::warn(
                        "Push constant range '{}' has inconsistent offset/size across shader stages ({}:{}) vs ({}:{})",
                        name,
                        existing.offset,
                        existing.size,
                        pc.offset,
                        pc.size);

                    has_push_constant_conflict = true;
                }

                const auto merged_stage_bits =
                    static_cast<std::underlying_type_t<ShaderStage>>(existing.stage) |
                    static_cast<std::underlying_type_t<ShaderStage>>(pc.stage);
                existing.stage = static_cast<ShaderStage>(merged_stage_bits);

                if (existing.members.empty() && !pc.members.empty())
                {
                    existing.members = pc.members;
                }
            }
        }

        if (has_push_constant_conflict)
        {
            Log::error("Cannot create pipeline layout due to conflicting push constant ranges");
            return false;
        }

        std::vector<VkPushConstantRange> push_constant_ranges;
        push_constant_ranges.reserve(push_constants_.size());

        for (const auto& pc : push_constants_ | std::views::values)
        {
            push_constant_ranges.emplace_back(
                to_vk(Flags<ShaderStage>{ pc.stage }),
                pc.offset,
                pc.size
            );
        }

        std::vector<VkDescriptorSetLayout> vk_set_layouts;
        vk_set_layouts.reserve(desc_.set_layouts.size());
        for (const auto* layout : desc_.set_layouts)
        {
            vk_set_layouts.push_back(reinterpret_cast<const DescriptorSetLayout*>(layout)->vk_descriptor_set_layout());
        }

        const VkPipelineLayoutCreateInfo layout_info
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = static_cast<std::uint32_t>(vk_set_layouts.size()),
            .pSetLayouts = vk_set_layouts.empty() ? nullptr : vk_set_layouts.data(),
            .pushConstantRangeCount = static_cast<std::uint32_t>(push_constant_ranges.size()),
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

        const auto* device = reinterpret_cast<Device*>(desc_.device);
        if (vk_pipeline_layout_)
        {
            vkDestroyPipelineLayout(device->logical_device(), vk_pipeline_layout_, nullptr);
            vk_pipeline_layout_ = nullptr;
        }
    }

    VkPipelineLayout PipelineLayout::vk_pipeline_layout() const { return vk_pipeline_layout_; }
}
