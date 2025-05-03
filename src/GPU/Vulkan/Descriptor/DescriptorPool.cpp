#include "DescriptorPool.hpp"

#include "GPU/Vulkan/Core/Device.hpp"
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

    VkDescriptorPool& DescriptorPool::get_descriptor_pool() { return instance().descriptor_pool; }
}
