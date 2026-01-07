module boza.rhi.vulkan;

import :resources;
import :util;

import <vk_all>;

namespace boza::rhi::vk
{
    bool Texture::init()
    {
        if (desc_.width == 0 || desc_.height == 0)
        {
            Log::warn("Texture created with zero dimensions, deferring initialization");
            return true;
        }

        const bool is_cube = desc_.type == TextureType::TextureCube || desc_.type == TextureType::TextureCubeArray;

        const VkImageCreateInfo image_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = is_cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : VkImageCreateFlags{},
            .imageType = to_vk_image_type(desc_.type),
            .format = to_vk(desc_.format),
            .extent = { desc_.width, desc_.height, desc_.depth },
            .mipLevels = desc_.mip_levels,
            .arrayLayers = desc_.array_layers * (is_cube ? 6 : 1),
            .samples = to_vk(desc_.sample_count),
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = to_vk(desc_.usage) | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        static constexpr VmaAllocationCreateInfo allocation_create_info
        {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

        const auto allocator = reinterpret_cast<Device*>(desc_.device)->allocator()->vma_allocator();

        if (!vk_check(
            vmaCreateImage(
                allocator, &image_create_info, &allocation_create_info,
                &image_, &allocation_, &allocation_info_),
            "Failed to create image"))
            return false;

        const VkImageViewCreateInfo image_view_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image_,
            .viewType = to_vk_image_view_type(desc_.type),
            .format = to_vk(desc_.format),
            .subresourceRange = {
                .aspectMask = get_image_aspect_flags(desc_.usage),
                .baseMipLevel = 0,
                .levelCount = desc_.mip_levels,
                .baseArrayLayer = 0,
                .layerCount = desc_.array_layers * (is_cube ? 6u : 1u),
            },
        };

        const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
        if (!vk_check(
            vkCreateImageView(vk_device, &image_view_create_info, nullptr, &image_view_),
            "Failed to create image view"))
            return false;

        if (desc_.usage.has(TextureUsage::Storage))
        {
            transition_layout_internal(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        }

        return true;
    }

    void Texture::destroy()
    {
        // Log::trace("Destroying texture");

        if (image_view_)
        {
            const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
            vkDestroyImageView(vk_device, image_view_, nullptr);
            image_view_ = nullptr;
        }

        if (image_)
        {
            const auto allocator = reinterpret_cast<Device*>(desc_.device)->allocator()->vma_allocator();
            vmaDestroyImage(allocator, image_, allocation_);
            image_ = nullptr;
            allocation_ = nullptr;
        }
    }

    void Texture::transition_layout_internal(const VkImageLayout old_layout, const VkImageLayout new_layout) const
    {
        // Log::trace("Transitioning texture layout: {} -> {}", static_cast<uint32_t>(old_layout), static_cast<uint32_t>(new_layout));

        const auto* device   = reinterpret_cast<Device*>(desc_.device);
        auto*       cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);

        auto* cmd_buffer = cmd_pool->begin_single_time_commands();
        if (!cmd_buffer)
        {
            Log::error("Failed to begin single-time commands for texture layout transition");
            return;
        }

        const bool is_cube = desc_.type == TextureType::TextureCube || desc_.type == TextureType::TextureCubeArray;
        const std::uint32_t layer_count = desc_.array_layers * (is_cube ? 6 : 1);

        VkImageMemoryBarrier barrier
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = {},
            .dstAccessMask = {},
            .oldLayout = old_layout,
            .newLayout = new_layout,
            .srcQueueFamilyIndex = vk_queue_family_ignored,
            .dstQueueFamilyIndex = vk_queue_family_ignored,
            .image = image_,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = desc_.mip_levels,
                .baseArrayLayer = 0,
                .layerCount = layer_count
            }
        };

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;

        // TODO: too long and may be repetitive, could be optimized

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            barrier.srcAccessMask = 0;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_GENERAL)
            {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from UNDEFINED to {}", static_cast<int>(new_layout));
                return;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_GENERAL)
            {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from TRANSFER_DST_OPTIMAL to {}", static_cast<int>(new_layout));
                return;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_GENERAL)
            {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from TRANSFER_SRC_OPTIMAL to {}", static_cast<int>(new_layout));
                return;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_GENERAL)
            {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            {
                barrier.dstAccessMask = 0;
                destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from SHADER_READ_ONLY_OPTIMAL to {}", static_cast<int>(new_layout));
                return;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_GENERAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from GENERAL to {}", static_cast<int>(new_layout));
                return;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            {
                barrier.dstAccessMask = 0;
                destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from COLOR_ATTACHMENT_OPTIMAL to {}", static_cast<int>(new_layout));
                return;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            barrier.srcAccessMask = 0;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from PRESENT_SRC_KHR to {}", static_cast<int>(new_layout));
                return;
            }
        }
        else
        {
            Log::error("Unsupported old layout: {}", static_cast<int>(old_layout));
            return;
        }

        vkCmdPipelineBarrier(
            reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer(),
            source_stage, destination_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        cmd_pool->end_single_time_commands(cmd_buffer);
    }

    void Texture::upload(const void* data, const size_t size, const std::uint32_t layer)
    {
        // Log::trace("Uploading {} bytes to texture layer {}", size, layer);

        const auto* device = reinterpret_cast<Device*>(desc_.device);

        Buffer staging_buffer{{
                .device = desc_.device,
                .size = size,
                .usage = BufferUsage::Staging,
                .memory_type = BufferMemoryType::HostVisible
        }};

        if (!staging_buffer.init())
        {
            Log::error("Failed to create staging buffer for texture upload");
            return;
        }

        staging_buffer.upload(data, size, 0);

        auto* cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);
        auto* cmd_buffer = cmd_pool->begin_single_time_commands();

        VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (desc_.usage.has(TextureUsage::Storage)) current_layout = VK_IMAGE_LAYOUT_GENERAL;

        {
            VkImageMemoryBarrier barrier
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = current_layout,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = vk_queue_family_ignored,
                .dstQueueFamilyIndex = vk_queue_family_ignored,
                .image = image_,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = layer,
                    .layerCount = 1
                }
            };

            vkCmdPipelineBarrier(
                reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer(),
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier
            );
        }

        const VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = layer,
                .layerCount = 1
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { desc_.width, desc_.height, 1 }
        };

        vkCmdCopyBufferToImage(
            reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer(),
            staging_buffer.vk_buffer(),
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        {
            const VkImageMemoryBarrier barrier
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = vk_queue_family_ignored,
                .dstQueueFamilyIndex = vk_queue_family_ignored,
                .image = image_,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = layer,
                    .layerCount = 1
                }
            };

            vkCmdPipelineBarrier(
                reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer(),
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrier
            );
        }

        cmd_pool->end_single_time_commands(cmd_buffer);


        staging_buffer.destroy();
    }

    void Texture::read_back(void* data, const size_t size, const std::uint32_t layer)
    {
        // Log::trace("Downloading {} bytes from texture", size);

        const auto* device = reinterpret_cast<Device*>(desc_.device);

        Buffer staging_buffer{{
            .device = desc_.device,
            .size = size,
            .usage = BufferUsage::Staging,
            .memory_type = BufferMemoryType::HostVisible
        }};

        if (!staging_buffer.init())
        {
            Log::error("Failed to create staging buffer for texture download");
            return;
        }

        transition_layout_internal(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        auto* cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);
        auto* cmd_buffer = cmd_pool->begin_single_time_commands();

        const VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = layer,
                .layerCount = 1
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { desc_.width, desc_.height, 1 }
        };

        vkCmdCopyImageToBuffer(
            reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer(),
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            staging_buffer.vk_buffer(),
            1,
            &region
        );

        cmd_pool->end_single_time_commands(cmd_buffer);

        staging_buffer.read_back(data, size, 0);

        transition_layout_internal(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        staging_buffer.destroy();
    }

    void Texture::transition_layout(const TextureLayout old_layout, const TextureLayout new_layout)
    {
        transition_layout_internal(to_vk_image_layout(old_layout), to_vk_image_layout(new_layout));
    }

    VkImage     Texture::vk_image() const { return image_; }
    VkImageView Texture::vk_image_view() const { return image_view_; }
}
