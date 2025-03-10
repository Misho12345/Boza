#include "DescriptorSet.hpp"
#include "../Device.hpp"
#include "Logger.hpp"

namespace boza
{
    bool DescriptorSet::create()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (const auto& buffer_info : buffer_infos)
        {
            VkDescriptorSetLayoutBinding binding
            {
                .binding = buffer_info.binding,
                .descriptorType = buffer_info.type,
                .descriptorCount = 1,
                .stageFlags = buffer_info.stage_flags,
                .pImmutableSamplers = nullptr
            };
            bindings.push_back(binding);
        }

        for (const auto& image_info : image_infos)
        {
            VkDescriptorSetLayoutBinding binding
            {
                .binding = image_info.binding,
                .descriptorType = image_info.type,
                .descriptorCount = 1,
                .stageFlags = image_info.stage_flags,
                .pImmutableSamplers = nullptr
            };
            bindings.push_back(binding);
        }

        const VkDescriptorSetLayoutCreateInfo layout_info
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        VK_CHECK(vkCreateDescriptorSetLayout(Device::get_device(), &layout_info, nullptr, &layout),
        {
            LOG_VK_ERROR("Failed to create descriptor set layout");
            return false;
        });

        descriptor_sets.resize(Swapchain::max_frames_in_flight);

        for (uint32_t i = 0; i < Swapchain::max_frames_in_flight; ++i)
        {
            VkDescriptorSetAllocateInfo alloc_info
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pNext = nullptr,
                .descriptorPool = DescriptorPool::get_descriptor_pool(),
                .descriptorSetCount = 1,
                .pSetLayouts = &layout
            };

            VK_CHECK(vkAllocateDescriptorSets(Device::get_device(), &alloc_info, &descriptor_sets[i]),
            {
                LOG_VK_ERROR("Failed to allocate descriptor set");
                return false;
            });

            update_descriptor_set(i);
        }

        return true;
    }

    void DescriptorSet::destroy()
    {
        for (auto& buffer_group : buffers)
        {
            for (auto& buffer : buffer_group)
                buffer.destroy();
        }
        buffers.clear();

        if (layout != nullptr)
        {
            vkDestroyDescriptorSetLayout(Device::get_device(), layout, nullptr);
            layout = nullptr;
        }
    }


    descriptor_set_binding DescriptorSet::add_image_sampler(const VkShaderStageFlags stage_flags)
    {
        const auto binding = static_cast<descriptor_set_binding>(buffer_infos.size() + image_infos.size());

        const ImageInfo info
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stage_flags = stage_flags,
            .image_view = nullptr,
            .sampler = nullptr,
            .binding = binding
        };

        image_infos.push_back(info);
        return binding;
    }

    void DescriptorSet::update_image_sampler(const descriptor_set_binding binding, Texture& texture)
    {
        if (binding >= buffer_infos.size() + image_infos.size()) return;

        for (auto& image_info : image_infos)
        {
            if (image_info.binding == binding)
            {
                image_info.image_view = texture.get_image_view();
                image_info.sampler = texture.get_sampler();
                break;
            }
        }

        for (uint32_t i = 0; i < Swapchain::max_frames_in_flight; ++i)
            update_descriptor_set(i);
    }


    VkDescriptorSet&       DescriptorSet::get_descriptor_set() { return descriptor_sets[Swapchain::current_frame_idx()]; }
    VkDescriptorSetLayout& DescriptorSet::get_layout() { return layout; }

    void DescriptorSet::update_descriptor_set(const uint32_t frame_index) const
    {
        std::vector<VkWriteDescriptorSet>   descriptor_writes;
        std::vector<VkDescriptorBufferInfo> buffer_descriptor_infos;
        std::vector<VkDescriptorImageInfo> image_descriptor_infos;

        buffer_descriptor_infos.resize(buffers.size());
        image_descriptor_infos.resize(image_infos.size());

        for (uint32_t i = 0; i < buffers.size(); ++i)
        {
            buffer_descriptor_infos[i] =
            {
                .buffer = buffers[i][frame_index].get_buffer(),
                .offset = 0,
                .range = buffer_infos[i].size,
            };

            VkWriteDescriptorSet write
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[frame_index],
                .dstBinding = buffer_infos[i].binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = buffer_infos[i].type,
                .pImageInfo = nullptr,
                .pBufferInfo = &buffer_descriptor_infos[i],
                .pTexelBufferView = nullptr
            };

            descriptor_writes.push_back(write);
        }

        for (uint32_t i = 0; i < image_infos.size(); ++i)
        {
            if (image_infos[i].image_view == nullptr || image_infos[i].sampler == nullptr)
                continue;

            image_descriptor_infos[i] =
            {
                .sampler = image_infos[i].sampler,
                .imageView = image_infos[i].image_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            VkWriteDescriptorSet write
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_sets[frame_index],
                .dstBinding = image_infos[i].binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = image_infos[i].type,
                .pImageInfo = &image_descriptor_infos[i],
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            };

            descriptor_writes.push_back(write);
        }

        vkUpdateDescriptorSets(
            Device::get_device(),
            static_cast<uint32_t>(descriptor_writes.size()),
            descriptor_writes.data(),
            0,
            nullptr);
    }
}
