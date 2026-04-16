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

        const auto variant_matches_type = [](const DescriptorType& type, const DescriptorInfo& info)
        {
            return std::visit(
                [type]<typename T>(const T&) -> bool
                {
                    using D = std::decay_t<T>;
                    if constexpr (std::same_as<D, UniformBuffer>) return type == DescriptorType::UniformBuffer;
                    else if constexpr (std::same_as<D, StorageBuffer>) return type == DescriptorType::StorageBuffer;
                    else if constexpr (std::same_as<D, CombinedImageSampler>) return type == DescriptorType::CombinedImageSampler;
                    else if constexpr (std::same_as<D, StorageImage>) return type == DescriptorType::StorageImage;
                    else return false;
                },
                info);
        };

        std::vector<VkWriteDescriptorSet> vk_writes;
        vk_writes.reserve(writes.size());

        std::vector<VkDescriptorBufferInfo> buffer_infos;
        buffer_infos.reserve(writes.size());

        std::vector<VkDescriptorImageInfo> image_infos;
        image_infos.reserve(writes.size());

        for (const auto& [binding, array_element, type, info] : writes)
        {
            if (!variant_matches_type(type, info))
            {
                Log::error("Descriptor write type mismatch at binding {}", binding);
                continue;
            }

            bool write_valid = true;

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
                        if (!arg.buffer)
                        {
                            Log::error("Descriptor write has null uniform buffer at binding {}", binding);
                            write_valid = false;
                            return;
                        }

                        buffer_infos.emplace_back(
                            static_cast<Buffer*>(arg.buffer)->vk_buffer(),
                            arg.offset,
                            arg.range
                        );
                        vk_write.pBufferInfo = &buffer_infos.back();
                    }
                    else if constexpr (std::same_as<decayed_t, StorageBuffer>)
                    {
                        if (!arg.buffer)
                        {
                            Log::error("Descriptor write has null storage buffer at binding {}", binding);
                            write_valid = false;
                            return;
                        }

                        buffer_infos.emplace_back(
                            static_cast<Buffer*>(arg.buffer)->vk_buffer(),
                            arg.offset,
                            arg.range
                        );
                        vk_write.pBufferInfo = &buffer_infos.back();
                    }
                    else if constexpr (std::same_as<decayed_t, CombinedImageSampler>)
                    {
                        if (!arg.texture || !arg.sampler)
                        {
                            Log::error("Descriptor write has null combined image sampler at binding {}", binding);
                            write_valid = false;
                            return;
                        }

                        image_infos.emplace_back(
                            static_cast<Sampler*>(arg.sampler)->vk_sampler(),
                            static_cast<Texture*>(arg.texture)->vk_image_view(),
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                        );
                        vk_write.pImageInfo = &image_infos.back();
                    }
                    else if constexpr (std::same_as<decayed_t, StorageImage>)
                    {
                        if (!arg.texture)
                        {
                            Log::error("Descriptor write has null storage image at binding {}", binding);
                            write_valid = false;
                            return;
                        }

                        image_infos.emplace_back(
                            nullptr,
                            static_cast<Texture*>(arg.texture)->vk_image_view(),
                            VK_IMAGE_LAYOUT_GENERAL
                        );
                        vk_write.pImageInfo = &image_infos.back();
                    }
                },
                info);

            if (!write_valid) continue;

            vk_writes.push_back(vk_write);
        }

        vkUpdateDescriptorSets(vk_device, static_cast<std::uint32_t>(vk_writes.size()), vk_writes.data(), 0, nullptr);
    }

    VkDescriptorSet DescriptorSet::vk_descriptor_set() const { return vk_descriptor_set_; }
    void            DescriptorSet::set_vk_descriptor_set(const VkDescriptorSet set) { vk_descriptor_set_ = set; }
}
