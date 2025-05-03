#pragma once
#include "DescriptorSet.hpp"
#include "DescriptorPool.hpp"
#include "GPU/Vulkan/Core/Swapchain.hpp"

namespace boza
{
    template<typename T>
    descriptor_set_binding DescriptorSet::add_uniform_buffer(const VkShaderStageFlags stage_flags)
    {
        const auto binding = static_cast<descriptor_set_binding>(buffer_infos.size() + image_infos.size());

        const BufferInfo info
        {
            .size = sizeof(T),
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stage_flags = stage_flags,
            .binding = binding
        };

        buffer_infos.push_back(info);

        std::vector<Buffer> buffer_group;
        buffer_group.resize(Swapchain::max_frames_in_flight);

        for (uint32_t i = 0; i < Swapchain::max_frames_in_flight; ++i)
            buffer_group[i] = Buffer::create_uniform_buffer(sizeof(T));

        buffers.push_back(std::move(buffer_group));
        return binding;
    }

    template<typename T>
    void DescriptorSet::update_buffer(const descriptor_set_binding binding, const T& data)
    {
        if (binding >= buffers.size() + image_infos.size()) return;

        for (uint32_t i = 0; i < buffer_infos.size(); ++i)
        {
            if (buffer_infos[i].binding == binding)
            {
                const uint32_t current_frame = Swapchain::current_frame_idx();
                buffers[i][current_frame].write(&data, sizeof(T), 0);
                break;
            }
        }
    }
}