module boza.rhi.vulkan;

import :descriptor;
import :util;

namespace boza::rhi::vk
{
    bool DescriptorPool::init()
    {
        // Log::trace("Creating vulkan descriptor pool");

        std::vector<VkDescriptorPoolSize> pool_sizes;
        pool_sizes.reserve(desc.pool_sizes.size());

        for (const auto& [type, count] : desc.pool_sizes)
        {
            pool_sizes.emplace_back(to_vk(type), count);
        }

        const VkDescriptorPoolCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = desc.max_sets,
            .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data()
        };

        const auto vk_device = static_cast<Device*>(desc.device)->logical_device();

        return vk_check(
            vkCreateDescriptorPool(vk_device, &create_info, nullptr, &vk_descriptor_pool_),
            "Failed to create descriptor pool");
    }

    void DescriptorPool::destroy()
    {
        // Log::trace("Destroying vulkan descriptor pool");

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (vk_descriptor_pool_)
        {
            vkDestroyDescriptorPool(vk_device, vk_descriptor_pool_, nullptr);
            vk_descriptor_pool_ = nullptr;
        }
    }


    rhi::DescriptorSet* DescriptorPool::allocate_descriptor_set(rhi::DescriptorSetLayout* layout)
    {
        // Log::trace("Allocating single descriptor set");

        return allocate_descriptor_sets(1, { layout })[0];
    }

    std::vector<rhi::DescriptorSet*> DescriptorPool::allocate_descriptor_sets(
        const uint32_t count,
        const std::vector<rhi::DescriptorSetLayout*>& layouts)
    {
        // Log::trace("Allocating {} descriptor set(s)", count);

        const auto vk_device = static_cast<Device*>(desc.device)->logical_device();

        std::vector<VkDescriptorSetLayout> vk_layouts;
        vk_layouts.reserve(layouts.size());
        for(const auto& layout : layouts)
        {
            vk_layouts.push_back(static_cast<DescriptorSetLayout*>(layout)->vk_descriptor_set_layout());
        }

        const VkDescriptorSetAllocateInfo alloc_info
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = vk_descriptor_pool_,
            .descriptorSetCount = count,
            .pSetLayouts = vk_layouts.data()
        };

        std::vector<VkDescriptorSet> vk_descriptor_sets(count);
        if (!vk_check(
            vkAllocateDescriptorSets(vk_device, &alloc_info, vk_descriptor_sets.data()),
            "Failed to allocate descriptor sets"))
            return {};

        std::vector<rhi::DescriptorSet*> result;
        result.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            DescriptorSetDesc set_desc{
                .device = desc.device,
                .pool = this,
                .layout = layouts[i]
            };

            auto* set = new DescriptorSet(set_desc);
            set->set_vk_descriptor_set(vk_descriptor_sets[i]);
            result.push_back(set);
        }

        return result;
    }


    void DescriptorPool::free_descriptor_set(rhi::DescriptorSet* set)
    {
        // Log::trace("Freeing single descriptor set");
        free_descriptor_sets({ set });
    }

    void DescriptorPool::free_descriptor_sets(const std::vector<rhi::DescriptorSet*>& sets)
    {
        // Log::trace("Freeing {} descriptor set(s)", sets.size());

        const auto vk_device = static_cast<Device*>(desc.device)->logical_device();

        std::vector<VkDescriptorSet> vk_descriptor_sets;
        vk_descriptor_sets.reserve(sets.size());

        for (const auto& set : sets)
        {
            vk_descriptor_sets.push_back(static_cast<DescriptorSet*>(set)->vk_descriptor_set());
            delete set;
        }

        vkFreeDescriptorSets(
            vk_device,
            vk_descriptor_pool_,
            static_cast<uint32_t>(vk_descriptor_sets.size()),
            vk_descriptor_sets.data());
    }


    bool DescriptorPool::reset()
    {
        // Log::trace("Resetting descriptor pool");

        const auto vk_device = static_cast<Device*>(desc.device)->logical_device();
        if (!vk_check(
            vkResetDescriptorPool(vk_device, vk_descriptor_pool_, 0),
            "Failed to reset descriptor pool"))
            return false;

        return true;
    }


    VkDescriptorPool DescriptorPool::vk_descriptor_pool() const { return vk_descriptor_pool_; }
}