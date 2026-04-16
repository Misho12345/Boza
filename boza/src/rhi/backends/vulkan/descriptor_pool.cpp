module boza.rhi.vulkan;

import :descriptor;
import :util;

namespace boza::rhi::vk
{
    bool DescriptorPool::init()
    {
        // Log::trace("Creating vulkan descriptor pool");

        std::vector<VkDescriptorPoolSize> pool_sizes;
        pool_sizes.reserve(desc_.pool_sizes.size());

        for (const auto& [type, count] : desc_.pool_sizes)
        {
            pool_sizes.emplace_back(to_vk(type), count);
        }

        const VkDescriptorPoolCreateInfo create_info
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = desc_.max_sets,
            .poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data()
        };

        const auto vk_device = static_cast<Device*>(desc_.device)->logical_device();

        return vk_check(
            vkCreateDescriptorPool(vk_device, &create_info, nullptr, &vk_descriptor_pool_),
            "Failed to create descriptor pool");
    }

    void DescriptorPool::destroy()
    {
        // Log::trace("Destroying vulkan descriptor pool");

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        if (vk_descriptor_pool_)
        {
            vkDestroyDescriptorPool(vk_device, vk_descriptor_pool_, nullptr);
            vk_descriptor_pool_ = nullptr;
        }
    }

    std::vector<rhi::DescriptorSet*> DescriptorPool::allocate_descriptor_sets(
        const std::uint32_t                        count,
        const std::span<rhi::DescriptorSetLayout*> layouts)
    {
        // Log::trace("Allocating {} descriptor set(s)", count);

        if (count == 0) return {};

        if (layouts.size() != count)
        {
            Log::critical(
                "Descriptor set allocation requires matching count/layouts (count: {}, layouts: {})",
                count,
                layouts.size());
            return {};
        }

        const auto vk_device = static_cast<Device*>(desc_.device)->logical_device();

        std::vector<VkDescriptorSetLayout> vk_layouts;
        vk_layouts.reserve(layouts.size());
        for (const auto& layout : layouts)
        {
            if (!layout)
            {
                Log::critical("Cannot allocate descriptor sets with null layout");
                return {};
            }

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

        for (std::uint32_t i = 0; i < count; ++i)
        {
            DescriptorSetDesc set_desc{
                .device = desc_.device,
                .pool = this,
                .layout = layouts[i]
            };

            auto* set = new DescriptorSet(set_desc);
            set->set_vk_descriptor_set(vk_descriptor_sets[i]);
            result.push_back(set);
        }

        return result;
    }

    void DescriptorPool::free_descriptor_sets(const std::span<rhi::DescriptorSet*> sets)
    {
        // Log::trace("Freeing {} descriptor set(s)", sets.size());

        if (sets.empty()) return;

        const auto vk_device = static_cast<Device*>(desc_.device)->logical_device();

        std::vector<VkDescriptorSet> vk_descriptor_sets;
        vk_descriptor_sets.reserve(sets.size());

        for (const auto& set : sets)
        {
            if (!set)
            {
                Log::warn("Skipping null descriptor set during free");
                continue;
            }

            vk_descriptor_sets.push_back(static_cast<DescriptorSet*>(set)->vk_descriptor_set());
        }

        if (vk_descriptor_sets.empty()) return;

        const VkResult free_result = vkFreeDescriptorSets(
            vk_device,
            vk_descriptor_pool_,
            static_cast<std::uint32_t>(vk_descriptor_sets.size()),
            vk_descriptor_sets.data());

        if (!vk_check(free_result, "Failed to free descriptor sets")) return;

        for (const auto& set : sets)
        {
            if (!set) continue;
            delete set;
        }
    }

    bool DescriptorPool::reset()
    {
        // Log::trace("Resetting descriptor pool");

        const auto vk_device = static_cast<Device*>(desc_.device)->logical_device();
        if (!vk_check(
            vkResetDescriptorPool(vk_device, vk_descriptor_pool_, 0),
            "Failed to reset descriptor pool"))
            return false;

        return true;
    }

    VkDescriptorPool DescriptorPool::vk_descriptor_pool() const { return vk_descriptor_pool_; }
}
