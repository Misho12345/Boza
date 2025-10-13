#include "Descriptor.hpp"
#include "Device.hpp"
#include "Resources.hpp"
#include "boza/core/Logger.hpp"

namespace boza::rhi::vk
{
    namespace
    {
        VkDescriptorType to_vk(const DescriptorType type)
        {
            switch (type)
            {
                case DescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
                case DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case DescriptorType::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                case DescriptorType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                case DescriptorType::UniformTexelBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                case DescriptorType::StorageTexelBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                case DescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case DescriptorType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                case DescriptorType::UniformBufferDynamic: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                case DescriptorType::StorageBufferDynamic: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                case DescriptorType::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            }

            std::unreachable();
        }

        VkShaderStageFlags to_vk(const Flags<ShaderStage> stages)
        {
            VkShaderStageFlags result = 0;

            if (stages.has(ShaderStage::Vertex)) result |= VK_SHADER_STAGE_VERTEX_BIT;
            if (stages.has(ShaderStage::Fragment)) result |= VK_SHADER_STAGE_FRAGMENT_BIT;
            if (stages.has(ShaderStage::Compute)) result |= VK_SHADER_STAGE_COMPUTE_BIT;
            if (stages.has(ShaderStage::TessControl)) result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            if (stages.has(ShaderStage::TessEvaluation)) result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            if (stages.has(ShaderStage::Geometry)) result |= VK_SHADER_STAGE_GEOMETRY_BIT;
            if (stages.has(ShaderStage::All)) result |= VK_SHADER_STAGE_ALL;

            return result;
        }
    }

    /// ---------------------------------
    /// ===== Descriptor Set Layout =====
    /// ---------------------------------

    bool DescriptorSetLayout::init()
    {
        // Logger::trace("Creating vulkan descriptor set layout");

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(desc.bindings.size());

        for (const auto& [binding, type, stages, count] : desc.bindings)
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

        const auto vk_device = static_cast<Device*>(desc.device)->logical_device();
        VK_CHECK(vkCreateDescriptorSetLayout(vk_device, &create_info, nullptr, &vk_descriptor_set_layout_),
        {
            LOG_VK_ERROR("Failed to create descriptor set layout");
            return false;
        });

        return true;
    }

    void DescriptorSetLayout::destroy()
    {
        // Logger::trace("Destroying vulkan descriptor set layout");

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (vk_descriptor_set_layout_)
        {
            vkDestroyDescriptorSetLayout(vk_device, vk_descriptor_set_layout_, nullptr);
            vk_descriptor_set_layout_ = nullptr;
        }
    }


    VkDescriptorSetLayout DescriptorSetLayout::vk_descriptor_set_layout() const { return vk_descriptor_set_layout_; }


    /// ---------------------------
    /// ===== Descriptor Pool =====
    /// ---------------------------

    bool DescriptorPool::init()
    {
        // Logger::trace("Creating vulkan descriptor pool");

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
        VK_CHECK(vkCreateDescriptorPool(vk_device, &create_info, nullptr, &vk_descriptor_pool_),
        {
            LOG_VK_ERROR("Failed to create descriptor pool");
            return false;
        });

        return true;
    }

    void DescriptorPool::destroy()
    {
        // Logger::trace("Destroying vulkan descriptor pool");

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (vk_descriptor_pool_)
        {
            vkDestroyDescriptorPool(vk_device, vk_descriptor_pool_, nullptr);
            vk_descriptor_pool_ = nullptr;
        }
    }


    rhi::DescriptorSet* DescriptorPool::allocate_descriptor_set(rhi::DescriptorSetLayout* layout)
    {
        // Logger::trace("Allocating single descriptor set");

        return allocate_descriptor_sets(1, { layout })[0];
    }

    std::vector<rhi::DescriptorSet*> DescriptorPool::allocate_descriptor_sets(
        const uint32_t count,
        const std::vector<rhi::DescriptorSetLayout*>& layouts)
    {
        // Logger::trace("Allocating {} descriptor set(s)", count);

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
        VK_CHECK(vkAllocateDescriptorSets(vk_device, &alloc_info, vk_descriptor_sets.data()),
        {
            LOG_VK_ERROR("Failed to allocate descriptor sets");
            return {};
        });

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
        // Logger::trace("Freeing single descriptor set");
        free_descriptor_sets({ set });
    }

    void DescriptorPool::free_descriptor_sets(const std::vector<rhi::DescriptorSet*>& sets)
    {
        // Logger::trace("Freeing {} descriptor set(s)", sets.size());

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
        // Logger::trace("Resetting descriptor pool");

        const auto vk_device = static_cast<Device*>(desc.device)->logical_device();
        VK_CHECK(vkResetDescriptorPool(vk_device, vk_descriptor_pool_, 0),
        {
            LOG_VK_ERROR("Failed to reset descriptor pool");
            return false;
        });

        return true;
    }


    VkDescriptorPool DescriptorPool::vk_descriptor_pool() const { return vk_descriptor_pool_; }


    /// --------------------------
    /// ===== Descriptor Set =====
    /// --------------------------

    bool DescriptorSet::init() { return true; }
    void DescriptorSet::destroy() {}

    void DescriptorSet::update(const std::vector<DescriptorWrite>& writes)
    {
        // Logger::trace("Updating descriptor set with {} write(s)", writes.size());

        const auto vk_device = static_cast<Device*>(desc.device)->logical_device();

        std::vector<VkWriteDescriptorSet> vk_writes;
        vk_writes.reserve(writes.size());

        std::vector<VkDescriptorBufferInfo> buffer_infos;
        buffer_infos.reserve(writes.size());

        std::vector<VkDescriptorImageInfo> image_infos;
        image_infos.reserve(writes.size());

        for (const auto& [binding, array_element, type, info] : writes)
        {
            VkWriteDescriptorSet vk_write
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = vk_descriptor_set_,
                .dstBinding = binding,
                .dstArrayElement = array_element,
                .descriptorCount = 1,
                .descriptorType = to_vk(type)
            };

            std::visit([&](auto&& arg)
                       {
                           using T = std::decay_t<decltype(arg)>;
                           if constexpr (std::is_same_v<T, UniformBuffer>)
                           {
                               buffer_infos.emplace_back(
                                   static_cast<Buffer*>(arg.buffer)->vk_buffer(),
                                   arg.offset,
                                   arg.range
                               );
                               vk_write.pBufferInfo = &buffer_infos.back();
                           }
                           else if constexpr (std::is_same_v<T, StorageBuffer>)
                           {
                               buffer_infos.emplace_back(
                                   static_cast<Buffer*>(arg.buffer)->vk_buffer(),
                                   arg.offset,
                                   arg.range
                               );
                               vk_write.pBufferInfo = &buffer_infos.back();
                           }
                           else if constexpr (std::is_same_v<T, CombinedImageSampler>)
                           {
                               image_infos.emplace_back(
                                   static_cast<Sampler*>(arg.sampler)->vk_sampler(),
                                   static_cast<Texture*>(arg.texture)->vk_image_view(),
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                               );
                               vk_write.pImageInfo = &image_infos.back();
                           }
                       },
                       info
            );

            vk_writes.push_back(vk_write);
        }

        vkUpdateDescriptorSets(vk_device, static_cast<uint32_t>(vk_writes.size()), vk_writes.data(), 0, nullptr);
    }

    VkDescriptorSet DescriptorSet::vk_descriptor_set() const { return vk_descriptor_set_; }
    void            DescriptorSet::set_vk_descriptor_set(const VkDescriptorSet set) { vk_descriptor_set_ = set; }
}
