#include "DescriptorSetLayout.hpp"

#include "../Device.hpp"
#include "Logger.hpp"

namespace boza
{
    DescriptorSetLayout DescriptorSetLayout::create_uniform_layout()
    {
        DescriptorSetLayout out;

        constexpr VkDescriptorSetLayoutBinding uniform_binding
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr
        };

        std::array bindings{ uniform_binding };

        const VkDescriptorSetLayoutCreateInfo layout_info
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        VK_CHECK(vkCreateDescriptorSetLayout(Device::get_device(), &layout_info, nullptr, &out.layout),
        {
            LOG_VK_ERROR("Failed to create descriptor set layout");
            return {};
        });

        return out;
    }

    DescriptorSetLayout DescriptorSetLayout::create_sampler_layout()
    {
        return {};
    }

    void DescriptorSetLayout::destroy() const
    {
        vkDestroyDescriptorSetLayout(Device::get_device(), layout, nullptr);
    }

    VkDescriptorSetLayout DescriptorSetLayout::get_layout() const { return layout; }
}
