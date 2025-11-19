module boza.rhi.vulkan;

import :resources;
import :util;

// TODO: make a tool to handle stbi outside of vulkan
import <vk_all.h>;
import <stb_image.h>;
import <stb_image_write.h>;

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
        // Log::trace("Creating texture ({}x{})", desc.width, desc.height);

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
            .extent = { desc.width, desc.height, 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
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
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (!vk_check(
            vkCreateImageView(vk_device, &image_view_create_info, nullptr, &image_view_),
            "Failed to create image view"))
            return false;

        if (desc.usage.has(TextureUsage::Storage))
        {
            transition_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
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
        }

        if (image_)
        {
            const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();
            vmaDestroyImage(allocator, image_, allocation_);
        }
    }

    void Texture::transition_layout(const VkImageLayout old_layout, const VkImageLayout new_layout) const
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

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            source_stage          = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage     = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout ==
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage          = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage          = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_GENERAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_GENERAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_GENERAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else
        {
            Log::error("Unsupported layout transition");
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

        const auto* device    = reinterpret_cast<Device*>(desc.device);
        const auto  allocator = device->allocator()->vma_allocator();

        const VkBufferCreateInfo buffer_info
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr
        };

        constexpr VmaAllocationCreateInfo alloc_info
        {
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        };

        VkBuffer      staging_buffer;
        VmaAllocation staging_allocation;

        if (!vk_check(
            vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &staging_buffer, &staging_allocation, nullptr),
            "Failed to create staging buffer for texture upload"))
            return;

        void* mapped_data;
        vmaMapMemory(allocator, staging_allocation, &mapped_data);
        std::memcpy(mapped_data, data, size);
        vmaUnmapMemory(allocator, staging_allocation);

        VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (desc.usage.has(TextureUsage::Storage)) current_layout = VK_IMAGE_LAYOUT_GENERAL;

        transition_layout(current_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        auto* cmd_pool   = device->command_pool(device->queue_family_indices().graphics_family);
        auto* cmd_buffer = cmd_pool->begin_single_time_commands();

        VkBufferImageCopy region
        {
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
            staging_buffer,
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        cmd_pool->end_single_time_commands(cmd_buffer);

        transition_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
    }

    bool Texture::load_from_file(const std::string& filepath)
    {
        // Log::trace("Loading texture from file: {}", filepath);

        int width, height, channels;

        if (!stbi_info(filepath.c_str(), &width, &height, &channels))
        {
            Log::error("Failed to query texture info from file: {}", filepath);
            return false;
        }

        int desired_channels = 4; // Default to RGBA
        switch (desc.format)
        {
            case TextureFormat::R8:
            case TextureFormat::R16F:
            case TextureFormat::R32F: desired_channels = 1;
                break;
            case TextureFormat::RG8:
            case TextureFormat::RG16F:
            case TextureFormat::RG32F: desired_channels = 2;
                break;
            case TextureFormat::RGB8:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB32F: desired_channels = 3;
                break;
            case TextureFormat::RGBA8:
            case TextureFormat::BGRA8:
            case TextureFormat::RGBA16F:
            case TextureFormat::RGBA32F: desired_channels = 4;
                break;
            case TextureFormat::DEPTH24STENCIL8:
            case TextureFormat::DEPTH32F: Log::warn("Cannot load depth/stencil texture from image file: {}", filepath);
                return false;
        }

        const int load_channels = std::max(channels, desired_channels);

        unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, load_channels);

        if (!pixels)
        {
            Log::error("Failed to load texture from file: {}", filepath);
            return false;
        }

        desc.width  = width;
        desc.height = height;

        if (image_)
        {
            const auto device    = reinterpret_cast<Device*>(desc.device)->logical_device();
            const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();

            if (image_view_)
            {
                vkDestroyImageView(device, image_view_, nullptr);
                image_view_ = nullptr;
            }

            vmaDestroyImage(allocator, image_, allocation_);
            image_      = nullptr;
            allocation_ = nullptr;
        }

        const VkImageCreateInfo image_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = to_vk(desc.format),
            .extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
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
        {
            stbi_image_free(pixels);
            return false;
        }

        const VkImageViewCreateInfo image_view_create_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image_,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = to_vk(desc.format),
            .subresourceRange = {
                .aspectMask = get_image_aspect_flags(desc.usage),
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
        if (!vk_check(
            vkCreateImageView(vk_device, &image_view_create_info, nullptr, &image_view_),
            "Failed to create image view"))
        {
            stbi_image_free(pixels);
            return false;
        }

        const size_t image_size = width * height * load_channels;
        upload(pixels, image_size);

        stbi_image_free(pixels);

        // Log::trace("Loaded texture from file: {} ({}x{}, {} channels)", filepath, width, height, load_channels);
        return true;
    }

    bool Texture::save_to_file(const std::string& filepath)
    {
        // TODO: make abstractions for buffer and move load and save as a non graphics api specific functionality
        const uint32_t width = desc.width;
        const uint32_t height = desc.height;

        int channels = 4;
        switch (desc.format)
        {
            // TODO: add all
            case TextureFormat::R8: channels = 1; break;
            case TextureFormat::RG8: channels = 2; break;
            case TextureFormat::RGB8: channels = 3; break;
            case TextureFormat::RGBA8:
            case TextureFormat::BGRA8: channels = 4; break;
            default:
                Log::error("Unsupported texture format for saving to file");
                return false;
        }

        const size_t image_size = width * height * channels;
        auto* pixels = new unsigned char[image_size];

        const auto staging_buffer = reinterpret_cast<Buffer*>(Buffer::create<Buffer>(
            {
                .device = desc.device,
                .size = image_size,
                .usage = BufferUsage::Staging,
                .memory_type = BufferMemoryType::HostCoherent,
                .is_constant = false
            }));

        auto* command_pool = desc.device->command_pool(desc.device->queue_family_indices().transfer_family);
        const auto cmd_buffer = reinterpret_cast<CommandBuffer*>(command_pool->begin_single_time_commands());

        const VkImageMemoryBarrier barrier_to_transfer
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = vk_queue_family_ignored,
            .dstQueueFamilyIndex = vk_queue_family_ignored,
            .image = image_,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            cmd_buffer->vk_command_buffer(),
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier_to_transfer);

        const VkBufferImageCopy copy_region
        {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { width, height, 1 },
        };

        vkCmdCopyImageToBuffer(
            cmd_buffer->vk_command_buffer(),
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            staging_buffer->vk_buffer(),
            1, &copy_region);

        const VkImageMemoryBarrier barrier_to_general
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = vk_queue_family_ignored,
            .dstQueueFamilyIndex = vk_queue_family_ignored,
            .image = image_,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            cmd_buffer->vk_command_buffer(),
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier_to_general);

        command_pool->end_single_time_commands(cmd_buffer);

        const void* mapped_data = staging_buffer->map();
        std::memcpy(pixels, mapped_data, image_size);
        staging_buffer->unmap();
        staging_buffer->destroy();

        const bool success = stbi_write_png(filepath.c_str(), width, height, channels, pixels, width * channels);
        delete[] pixels;

        if (!success)
        {
            Log::error("Failed to write texture to file: {}", filepath);
            return false;
        }

        Log::trace("Saved texture to file: {} ({}x{}, {} channels)", filepath, width, height, channels);
        return true;
    }

    void Texture::transition_layout_external()
    {
        if (desc.usage.has(TextureUsage::Storage) && desc.usage.has(TextureUsage::Sampled))
        {
            transition_layout(VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    VkImage     Texture::vk_image() const { return image_; }
    VkImageView Texture::vk_image_view() const { return image_view_; }
}
