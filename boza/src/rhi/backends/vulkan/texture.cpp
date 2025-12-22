module boza.rhi.vulkan;

import :resources;
import :util;

import <vk_all>;

namespace boza::rhi::vk
{
    VkFormat to_vk(const TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
            case TextureFormat::RG8: return VK_FORMAT_R8G8_UNORM;
            case TextureFormat::RGB8: return VK_FORMAT_R8G8B8_UNORM;
            case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
            case TextureFormat::R16F: return VK_FORMAT_R16_SFLOAT;
            case TextureFormat::RG16F: return VK_FORMAT_R16G16_SFLOAT;
            case TextureFormat::RGB16F: return VK_FORMAT_R16G16B16_SFLOAT;
            case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case TextureFormat::R32F: return VK_FORMAT_R32_SFLOAT;
            case TextureFormat::RG32F: return VK_FORMAT_R32G32_SFLOAT;
            case TextureFormat::RGB32F: return VK_FORMAT_R32G32B32_SFLOAT;
            case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case TextureFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
            case TextureFormat::DEPTH32F: return VK_FORMAT_D32_SFLOAT;
        }

        std::unreachable();
    }

    VkImageUsageFlags to_vk(const Flags<TextureUsage> usage)
    {
        VkImageUsageFlags result = 0;

        if (usage.has(TextureUsage::Sampled)) result |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (usage.has(TextureUsage::Storage)) result |= VK_IMAGE_USAGE_STORAGE_BIT;
        if (usage.has(TextureUsage::ColorAttachment)) result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (usage.has(TextureUsage::DepthStencilAttachment)) result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if (usage.has(TextureUsage::TransferSrc)) result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if (usage.has(TextureUsage::TransferDst)) result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (usage.has(TextureUsage::InputAttachment)) result |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

        return result;
    }

    VkImageAspectFlags get_image_aspect_flags(const Flags<TextureUsage> usage)
    {
        if (usage.has(TextureUsage::DepthStencilAttachment)) return VK_IMAGE_ASPECT_DEPTH_BIT;
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageAspectFlags get_image_aspect_flags(const TextureUsage usage)
    {
        switch (usage)
        {
            case TextureUsage::Sampled:
            case TextureUsage::Storage:
            case TextureUsage::ColorAttachment:
            case TextureUsage::TransferSrc:
            case TextureUsage::TransferDst:
            case TextureUsage::InputAttachment: return VK_IMAGE_ASPECT_COLOR_BIT;
            case TextureUsage::DepthStencilAttachment: return VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        std::unreachable();
    }


    bool Texture::init()
    {
        if (desc.width == 0 || desc.height == 0)
        {
            Log::warn("Texture created with zero dimensions, deferring initialization");
            return true;
        }

        const VkImageCreateInfo image_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = to_vk(desc.format),
            .extent = { desc.width, desc.height, desc.depth },
            .mipLevels = desc.mip_levels,
            .arrayLayers = desc.array_layers,
            .samples = static_cast<VkSampleCountFlagBits>(desc.sample_count),
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = to_vk(desc.usage) | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        constexpr VmaAllocationCreateInfo allocation_create_info
        {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

        const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();

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
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = to_vk(desc.format),
            .subresourceRange = {
                .aspectMask = get_image_aspect_flags(desc.usage),
                .baseMipLevel = 0,
                .levelCount = desc.mip_levels,
                .baseArrayLayer = 0,
                .layerCount = desc.array_layers,
            },
        };

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (!vk_check(
            vkCreateImageView(vk_device, &image_view_create_info, nullptr, &image_view_),
            "Failed to create image view"))
            return false;

        if (desc.usage.has(TextureUsage::Storage))
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
            const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
            vkDestroyImageView(vk_device, image_view_, nullptr);
            image_view_ = nullptr;
        }

        if (image_)
        {
            const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();
            vmaDestroyImage(allocator, image_, allocation_);
            image_ = nullptr;
            allocation_ = nullptr;
        }
    }

    VkImageLayout to_vk(const TextureLayout layout)
    {
        switch (layout)
        {
            case TextureLayout::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
            case TextureLayout::General: return VK_IMAGE_LAYOUT_GENERAL;
            case TextureLayout::ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case TextureLayout::DepthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case TextureLayout::ShaderReadOnly: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case TextureLayout::TransferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            case TextureLayout::TransferDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case TextureLayout::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
        std::unreachable();
    }

    void Texture::transition_layout_internal(const VkImageLayout old_layout, const VkImageLayout new_layout) const
    {
        // Log::trace("Transitioning texture layout: {} -> {}", static_cast<uint32_t>(old_layout), static_cast<uint32_t>(new_layout));

        const auto* device   = reinterpret_cast<Device*>(desc.device);
        auto*       cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);

        auto* cmd_buffer = cmd_pool->begin_single_time_commands();
        if (!cmd_buffer)
        {
            Log::error("Failed to begin single-time commands for texture layout transition");
            return;
        }

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
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;

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

    void Texture::upload(const void* data, const size_t size)
    {
        // Log::trace("Uploading {} bytes to texture", size);

        const auto* device = reinterpret_cast<Device*>(desc.device);

        Buffer staging_buffer{ BufferDesc{
            .device = desc.device,
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

        VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (desc.usage.has(TextureUsage::Storage)) current_layout = VK_IMAGE_LAYOUT_GENERAL;

        transition_layout_internal(current_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        auto* cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);
        auto* cmd_buffer = cmd_pool->begin_single_time_commands();

        const VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { desc.width, desc.height, 1 }
        };

        vkCmdCopyBufferToImage(
            reinterpret_cast<CommandBuffer*>(cmd_buffer)->vk_command_buffer(),
            staging_buffer.vk_buffer(),
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        cmd_pool->end_single_time_commands(cmd_buffer);

        transition_layout_internal(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        staging_buffer.destroy();
    }

    void Texture::download(void* data, const size_t size)
    {
        // Log::trace("Downloading {} bytes from texture", size);

        const auto* device = reinterpret_cast<Device*>(desc.device);

        Buffer staging_buffer{ BufferDesc{
            .device = desc.device,
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
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { desc.width, desc.height, 1 }
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
        transition_layout_internal(to_vk(old_layout), to_vk(new_layout));
    }

    VkImage     Texture::vk_image() const { return image_; }
    VkImageView Texture::vk_image_view() const { return image_view_; }
}
