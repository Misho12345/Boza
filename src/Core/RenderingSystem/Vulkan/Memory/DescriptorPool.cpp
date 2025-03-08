#include "DescriptorPool.hpp"

#include "../Device.hpp"
#include "Logger.hpp"

namespace boza
{
    bool DescriptorPool::create()
    {
        std::array pool_sizes
        {
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }
        };

        const VkDescriptorPoolCreateInfo descriptor_pool_info
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .maxSets = 10,
            .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data()
        };

        VK_CHECK(vkCreateDescriptorPool(Device::get_device(), &descriptor_pool_info, nullptr, &instance().descriptor_pool),
        {
            LOG_VK_ERROR("Failed to create descriptor pool");
            return false;
        });

        return true;
    }

    void DescriptorPool::destroy()
    {
        vkDestroyDescriptorPool(Device::get_device(), instance().descriptor_pool, nullptr);
    }

    VkDescriptorSet DescriptorPool::create_descriptor_set(const DescriptorSetLayout& layout)
    {
        const VkDescriptorSetLayout layout_ = layout.get_layout();
        const VkDescriptorSetAllocateInfo alloc_info
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = {},
            .descriptorPool = get_descriptor_pool(),
            .descriptorSetCount = 1,
            .pSetLayouts = &layout_
        };

        VkDescriptorSet descriptor_set;
        VK_CHECK(vkAllocateDescriptorSets(Device::get_device(), &alloc_info, &descriptor_set),
        {
            LOG_VK_ERROR("Failed to allocate descriptor set");
            return {};
        });

        return descriptor_set;
    }

    VkDescriptorPool& DescriptorPool::get_descriptor_pool() { return instance().descriptor_pool; }
}
