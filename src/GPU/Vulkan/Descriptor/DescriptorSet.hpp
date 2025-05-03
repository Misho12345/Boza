#pragma once
#include "boza_pch.hpp"

#include "GPU/Vulkan/Memory/Buffer.hpp"
#include "GPU/Vulkan/Memory/Texture.hpp"

namespace boza
{
    using descriptor_set_binding = uint32_t;
    class DescriptorSet final
    {
    public:
        DescriptorSet() = default;

        bool create();
        void destroy();

        template<typename T> descriptor_set_binding add_uniform_buffer(VkShaderStageFlags stage_flags);
        template<typename T> void update_buffer(descriptor_set_binding binding, const T& data);

        descriptor_set_binding add_image_sampler(VkShaderStageFlags stage_flags);
        void update_image_sampler(descriptor_set_binding binding, Texture& texture);

        VkDescriptorSet& get_descriptor_set();
        VkDescriptorSetLayout& get_layout();

    private:
        struct BufferInfo
        {
            VkDeviceSize           size;
            VkDescriptorType       type;
            VkShaderStageFlags     stage_flags;
            descriptor_set_binding binding;
        };

        struct ImageInfo
        {
            VkDescriptorType       type;
            VkShaderStageFlags     stage_flags;
            VkImageView            image_view;
            VkSampler              sampler;
            descriptor_set_binding binding;
        };

        void update_descriptor_set(uint32_t frame_index) const;

        std::vector<std::vector<Buffer>> buffers;
        std::vector<BufferInfo>          buffer_infos;
        std::vector<ImageInfo>           image_infos;
        std::vector<VkDescriptorSet>     descriptor_sets;

        VkDescriptorSetLayout layout{ nullptr };
    };
}

#include "DescriptorSet.inl"
