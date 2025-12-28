module boza.rhi.vulkan;

import :descriptor;
import :util;

namespace boza::rhi::vk
{
    bool DescriptorSetLayout::init()
    {
        // Log::trace("Creating vulkan descriptor set layout");

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(desc_.bindings.size());

        for (const auto& [binding, type, stages, count] : desc_.bindings)
        {
            bindings.emplace_back(
                binding,
                to_vk(type),
                count,
                to_vk(stages),
                nullptr
            );
        }

        const VkDescriptorSetLayoutCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        const auto vk_device = static_cast<Device*>(desc_.device)->logical_device();
        if (!vk_check(
            vkCreateDescriptorSetLayout(vk_device, &create_info, nullptr, &vk_descriptor_set_layout_),
            "Failed to create descriptor set layout"))
            return false;

        return true;
    }

    void DescriptorSetLayout::destroy()
    {
        // Log::trace("Destroying vulkan descriptor set layout");

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        if (vk_descriptor_set_layout_)
        {
            vkDestroyDescriptorSetLayout(vk_device, vk_descriptor_set_layout_, nullptr);
            vk_descriptor_set_layout_ = nullptr;
        }
    }

    VkDescriptorSetLayout DescriptorSetLayout::vk_descriptor_set_layout() const { return vk_descriptor_set_layout_; }
}
