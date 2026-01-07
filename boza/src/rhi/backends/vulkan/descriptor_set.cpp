module boza.rhi.vulkan;

import :descriptor;
import :util;

namespace boza::rhi::vk
{
    bool DescriptorSet::init() { return true; }
    void DescriptorSet::destroy() {}

    void DescriptorSet::update(const std::span<DescriptorWrite> writes)
    {
        // Log::trace("Updating descriptor set with {} write(s)", writes.size());

        const auto vk_device = static_cast<Device*>(desc_.device)->logical_device();

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

            std::visit(
                [&]<typename T>(T&& arg)
                {
                    using decayed_t = std::decay_t<T>;
                    if constexpr (std::same_as<decayed_t, UniformBuffer>)
                    {
                        buffer_infos.emplace_back(
                            static_cast<Buffer*>(arg.buffer)->vk_buffer(),
                            arg.offset,
                            arg.range
                        );
                        vk_write.pBufferInfo = &buffer_infos.back();
                    }
                    else if constexpr (std::same_as<decayed_t, StorageBuffer>)
                    {
                        buffer_infos.emplace_back(
                            static_cast<Buffer*>(arg.buffer)->vk_buffer(),
                            arg.offset,
                            arg.range
                        );
                        vk_write.pBufferInfo = &buffer_infos.back();
                    }
                    else if constexpr (std::same_as<decayed_t, CombinedImageSampler>)
                    {
                        image_infos.emplace_back(
                            static_cast<Sampler*>(arg.sampler)->vk_sampler(),
                            static_cast<Texture*>(arg.texture)->vk_image_view(),
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                        );
                        vk_write.pImageInfo = &image_infos.back();
                    }
                    else if constexpr (std::same_as<decayed_t, StorageImage>)
                    {
                        image_infos.emplace_back(
                            nullptr,
                            static_cast<Texture*>(arg.texture)->vk_image_view(),
                            VK_IMAGE_LAYOUT_GENERAL
                        );
                        vk_write.pImageInfo = &image_infos.back();
                    }
                },
                info);

            vk_writes.push_back(vk_write);
        }

        vkUpdateDescriptorSets(vk_device, static_cast<uint32_t>(vk_writes.size()), vk_writes.data(), 0, nullptr);
    }

    VkDescriptorSet DescriptorSet::vk_descriptor_set() const { return vk_descriptor_set_; }
    void            DescriptorSet::set_vk_descriptor_set(const VkDescriptorSet set) { vk_descriptor_set_ = set; }
}
