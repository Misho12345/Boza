module boza.rhi.vulkan;

import :resources;
import :util;

import <vk_all>;

namespace boza::rhi::vk
{
    namespace
    {
        constexpr std::size_t bytes_per_pixel(const TextureFormat format)
        {
            switch (format)
            {
                case TextureFormat::Undefined: return 4;
                case TextureFormat::R8: return 1;
                case TextureFormat::RG8: return 2;
                case TextureFormat::RGB8: return 3;
                case TextureFormat::RGBA8: return 4;
                case TextureFormat::BGRA8: return 4;
                case TextureFormat::R16F: return 2;
                case TextureFormat::RG16F: return 4;
                case TextureFormat::RGB16F: return 6;
                case TextureFormat::RGBA16F: return 8;
                case TextureFormat::R32F: return 4;
                case TextureFormat::RG32F: return 8;
                case TextureFormat::RGB32F: return 12;
                case TextureFormat::RGBA32F: return 16;
                case TextureFormat::DEPTH24STENCIL8: return 4;
                case TextureFormat::DEPTH32F: return 4;
            }

            return 4;
        }

        constexpr std::uint32_t copy_depth_for_type(const TextureType type, const std::uint32_t depth)
        {
            if (type == TextureType::Texture3D) return std::max(depth, 1u);
            return 1u;
        }

        VkImageAspectFlags image_aspect_flags_for_desc(const TextureDesc& desc)
        {
            if (!(desc.usage & TextureUsage::DepthStencilAttachment)) return VK_IMAGE_ASPECT_COLOR_BIT;

            VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (desc.format == TextureFormat::DEPTH24STENCIL8) aspect_flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
            return aspect_flags;
        }
    }

    bool Texture::init()
    {
        if (desc_.width == 0 || desc_.height == 0)
        {
            Log::warn("Texture created with zero dimensions, deferring initialization");
            return true;
        }

        const auto* device = reinterpret_cast<Device*>(desc_.device);

        if (desc_.type == TextureType::TextureCubeArray && !device->image_cube_array_enabled())
        {
            Log::error("TextureCubeArray requires Vulkan imageCubeArray support, but it is not enabled");
            return false;
        }

        const bool is_cube = desc_.type == TextureType::TextureCube || desc_.type == TextureType::TextureCubeArray;
        const std::uint32_t total_layers = desc_.array_layers * (is_cube ? 6u : 1u);

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
            .usage = to_vk(desc_.usage) | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        static constexpr VmaAllocationCreateInfo allocation_create_info
        {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

        const auto allocator = device->allocator()->vma_allocator();

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
                .aspectMask = image_aspect_flags_for_desc(desc_),
                .baseMipLevel = 0,
                .levelCount = desc_.mip_levels,
                .baseArrayLayer = 0,
                .layerCount = total_layers,
            },
        };

        const auto vk_device = device->logical_device();
        if (!vk_check(
            vkCreateImageView(vk_device, &image_view_create_info, nullptr, &image_view_),
            "Failed to create image view"))
            return false;

        if (total_layers > 1)
        {
            layer_image_views_.reserve(total_layers);

            for (std::uint32_t layer = 0; layer < total_layers; ++layer)
            {
                const VkImageViewCreateInfo layer_image_view_create_info
                {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = image_,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = to_vk(desc_.format),
                    .subresourceRange = {
                        .aspectMask = image_aspect_flags_for_desc(desc_),
                        .baseMipLevel = 0,
                        .levelCount = desc_.mip_levels,
                        .baseArrayLayer = layer,
                        .layerCount = 1,
                    },
                };

                VkImageView layer_image_view{ nullptr };
                if (!vk_check(
                    vkCreateImageView(vk_device, &layer_image_view_create_info, nullptr, &layer_image_view),
                    "Failed to create texture layer image view"))
                    return false;

                layer_image_views_.push_back(layer_image_view);
            }
        }

        layer_layouts_.assign(total_layers, VK_IMAGE_LAYOUT_UNDEFINED);

        if (desc_.usage & TextureUsage::Storage)
        {
            transition_layout_internal(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
            std::ranges::fill(layer_layouts_, VK_IMAGE_LAYOUT_GENERAL);
        }

        return true;
    }

    void Texture::destroy()
    {
        // Log::trace("Destroying texture");

        if (image_view_)
        {
            const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
            for (VkImageView layer_image_view : layer_image_views_)
            {
                vkDestroyImageView(vk_device, layer_image_view, nullptr);
            }
            layer_image_views_.clear();

            vkDestroyImageView(vk_device, image_view_, nullptr);
            image_view_ = nullptr;
        }
        else
        {
            layer_image_views_.clear();
        }

        if (image_)
        {
            const auto allocator = reinterpret_cast<Device*>(desc_.device)->allocator()->vma_allocator();
            vmaDestroyImage(allocator, image_, allocation_);
            image_ = nullptr;
            allocation_ = nullptr;
        }

        layer_layouts_.clear();
    }

    std::unique_ptr<rhi::Buffer> Texture::stage(const std::size_t size, const std::uint32_t layer) const
    {
        const bool is_cube = desc_.type == TextureType::TextureCube || desc_.type == TextureType::TextureCubeArray;
        const std::uint32_t total_layers = desc_.array_layers * (is_cube ? 6u : 1u);
        if (layer >= total_layers)
        {
            Log::error("Texture staging layer {} out of bounds ({} layers)", layer, total_layers);
            return nullptr;
        }

        const std::uint32_t copy_depth = copy_depth_for_type(desc_.type, desc_.depth);

        const std::size_t required_size =
            static_cast<std::size_t>(desc_.width) *
            static_cast<std::size_t>(desc_.height) *
            static_cast<std::size_t>(copy_depth) *
            bytes_per_pixel(desc_.format);

        const std::size_t stage_size = size > 0 ? size : required_size;
        if (stage_size < required_size)
        {
            Log::error(
                "Staging buffer size {} is too small for texture layer copy (required {})",
                stage_size,
                required_size);
            return nullptr;
        }

        const BufferDesc staging_desc
        {
            .device = desc_.device,
            .size = stage_size,
            .usage = BufferUsage::Staging,
            .memory_type = BufferMemoryType::HostVisible | BufferMemoryType::HostCoherent
        };

        auto staging_buffer = Buffer::create<Buffer>(staging_desc);
        if (!staging_buffer)
        {
            Log::error("Failed to create staging buffer for texture operation");
            return nullptr;
        }

        if (layer >= layer_layouts_.size())
        {
            Log::error("Texture layer {} layout state is unavailable", layer);
            return nullptr;
        }

        const VkImageLayout current_layout = layer_layouts_[layer];
        const VkImageAspectFlags aspect_mask = image_aspect_flags_for_desc(desc_);

        const auto* device = reinterpret_cast<Device*>(desc_.device);
        auto* cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);
        if (!cmd_pool)
        {
            Log::error("Failed to get graphics command pool for texture staging");
            return nullptr;
        }

        auto* cmd_buffer = cmd_pool->begin_single_time_commands();
        if (!cmd_buffer)
        {
            Log::error("Failed to begin single-time commands for texture staging");
            return nullptr;
        }

        const auto* vk_cmd_buffer = reinterpret_cast<CommandBuffer*>(cmd_buffer);
        vk_cmd_buffer->pipeline_image_barrier(
            image_,
            current_layout,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT,
            aspect_mask,
            0,
            1,
            layer,
            1);

        const VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = aspect_mask,
                .mipLevel = 0,
                .baseArrayLayer = layer,
                .layerCount = 1
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { desc_.width, desc_.height, copy_depth }
        };

        vkCmdCopyImageToBuffer(
            vk_cmd_buffer->vk_command_buffer(),
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            reinterpret_cast<Buffer*>(staging_buffer.get())->vk_buffer(),
            1,
            &region
        );

        vk_cmd_buffer->pipeline_image_barrier(
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            current_layout,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            aspect_mask,
            0,
            1,
            layer,
            1);

        if (!cmd_pool->end_single_time_commands(cmd_buffer))
        {
            Log::error("Failed to end single-time commands for texture staging");
            return nullptr;
        }

        return staging_buffer;
    }

    void Texture::transition_layout_internal(const VkImageLayout old_layout, const VkImageLayout new_layout) const
    {
        // Log::trace("Transitioning texture layout: {} -> {}", static_cast<std::uint32_t>(old_layout), static_cast<std::uint32_t>(new_layout));

        const auto* device   = reinterpret_cast<Device*>(desc_.device);
        auto*       cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);
        if (!cmd_pool)
        {
            Log::error("Failed to get graphics command pool for texture layout transition");
            return;
        }

        const bool is_cube = desc_.type == TextureType::TextureCube || desc_.type == TextureType::TextureCubeArray;
        const std::uint32_t layer_count = desc_.array_layers * (is_cube ? 6 : 1);

        VkAccessFlags2 src_access = VK_ACCESS_2_NONE;
        VkAccessFlags2 dst_access = VK_ACCESS_2_NONE;
        VkPipelineStageFlags2 source_stage = VK_PIPELINE_STAGE_2_NONE;
        VkPipelineStageFlags2 destination_stage = VK_PIPELINE_STAGE_2_NONE;
        bool transition_supported = true;

        // TODO: too long and may be repetitive, could be optimized

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            src_access = VK_ACCESS_2_NONE;
            source_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_GENERAL)
            {
                dst_access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from UNDEFINED to {}", static_cast<int>(new_layout));
                transition_supported = false;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            src_access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_GENERAL)
            {
                dst_access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from TRANSFER_DST_OPTIMAL to {}", static_cast<int>(new_layout));
                transition_supported = false;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            src_access = VK_ACCESS_2_TRANSFER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_GENERAL)
            {
                dst_access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from TRANSFER_SRC_OPTIMAL to {}", static_cast<int>(new_layout));
                transition_supported = false;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            src_access = VK_ACCESS_2_SHADER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_GENERAL)
            {
                dst_access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            {
                dst_access = VK_ACCESS_2_NONE;
                destination_stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from SHADER_READ_ONLY_OPTIMAL to {}", static_cast<int>(new_layout));
                transition_supported = false;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_GENERAL)
        {
            src_access = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from GENERAL to {}", static_cast<int>(new_layout));
                transition_supported = false;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            src_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_SHADER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_READ_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            {
                dst_access = VK_ACCESS_2_NONE;
                destination_stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from COLOR_ATTACHMENT_OPTIMAL to {}", static_cast<int>(new_layout));
                transition_supported = false;
            }
        }
        else if (old_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            src_access = VK_ACCESS_2_NONE;
            source_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

            if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
            }
            else if (new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                dst_access = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
                destination_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else
            {
                Log::error("Unsupported layout transition from PRESENT_SRC_KHR to {}", static_cast<int>(new_layout));
                transition_supported = false;
            }
        }
        else
        {
            Log::error("Unsupported old layout: {}", static_cast<int>(old_layout));
            transition_supported = false;
        }

        if (!transition_supported) return;

        auto* cmd_buffer = cmd_pool->begin_single_time_commands();
        if (!cmd_buffer)
        {
            Log::error("Failed to begin single-time commands for texture layout transition");
            return;
        }

        const auto* vk_cmd_buffer = reinterpret_cast<CommandBuffer*>(cmd_buffer);
        const VkImageAspectFlags aspect_mask = image_aspect_flags_for_desc(desc_);
        vk_cmd_buffer->pipeline_image_barrier(
            image_,
            old_layout,
            new_layout,
            source_stage,
            src_access,
            destination_stage,
            dst_access,
            aspect_mask,
            0,
            desc_.mip_levels,
            0,
            layer_count);

        if (!cmd_pool->end_single_time_commands(cmd_buffer))
        {
            Log::error("Failed to submit texture layout transition commands");
            return;
        }

        std::ranges::fill(layer_layouts_, new_layout);
    }

    void Texture::upload(const void* data, const std::size_t size, const std::uint32_t layer)
    {
        // Log::trace("Uploading {} bytes to texture layer {}", size, layer);

        const BufferDesc staging_desc
        {
            .device = desc_.device,
            .size = size,
            .usage = BufferUsage::Staging,
            .memory_type = BufferMemoryType::HostVisible | BufferMemoryType::HostCoherent
        };

        const auto staging_buffer = Buffer::create<Buffer>(staging_desc);
        if (!staging_buffer) return;

        staging_buffer->upload(data, size, 0);

        if (!upload_from(staging_buffer.get(), size, layer))
        {
            Log::error("Failed to upload texture data from staging buffer");
        }
    }

    bool Texture::upload_from(rhi::Buffer* staging_buffer, const std::size_t size, const std::uint32_t layer)
    {
        if (!staging_buffer)
        {
            Log::error("Cannot upload texture from null staging buffer");
            return false;
        }

        const bool is_cube = desc_.type == TextureType::TextureCube || desc_.type == TextureType::TextureCubeArray;
        const std::uint32_t total_layers = desc_.array_layers * (is_cube ? 6u : 1u);
        if (layer >= total_layers)
        {
            Log::error("Texture upload layer {} out of bounds ({} layers)", layer, total_layers);
            return false;
        }

        const std::uint32_t copy_depth = copy_depth_for_type(desc_.type, desc_.depth);

        const std::size_t required_size =
            static_cast<std::size_t>(desc_.width) *
            static_cast<std::size_t>(desc_.height) *
            static_cast<std::size_t>(copy_depth) *
            bytes_per_pixel(desc_.format);

        const std::size_t transfer_size = size > 0 ? size : staging_buffer->size();
        if (transfer_size < required_size)
        {
            Log::error(
                "Staging upload size {} is too small for texture layer upload (required {})",
                transfer_size,
                required_size);
            return false;
        }

        if (layer >= layer_layouts_.size())
        {
            Log::error("Texture upload layer {} layout state is unavailable", layer);
            return false;
        }

        const auto* device = reinterpret_cast<Device*>(desc_.device);

        auto* cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);
        if (!cmd_pool)
        {
            Log::error("Failed to get graphics command pool for texture upload");
            return false;
        }

        auto* cmd_buffer = cmd_pool->begin_single_time_commands();
        if (!cmd_buffer)
        {
            Log::error("Failed to begin single-time commands for texture upload");
            return false;
        }

        const VkImageLayout current_layout = layer_layouts_[layer];

        const VkImageLayout final_layout = desc_.usage & TextureUsage::Storage
                                               ? VK_IMAGE_LAYOUT_GENERAL
                                               : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        const auto* vk_cmd_buffer = reinterpret_cast<CommandBuffer*>(cmd_buffer);
        const VkImageAspectFlags aspect_mask = image_aspect_flags_for_desc(desc_);
        vk_cmd_buffer->pipeline_image_barrier(
            image_,
            current_layout,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            aspect_mask,
            0,
            1,
            layer,
            1);

        const VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = aspect_mask,
                .mipLevel = 0,
                .baseArrayLayer = layer,
                .layerCount = 1
            },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { desc_.width, desc_.height, copy_depth }
        };

        vkCmdCopyBufferToImage(
            vk_cmd_buffer->vk_command_buffer(),
            static_cast<Buffer*>(staging_buffer)->vk_buffer(),
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        vk_cmd_buffer->pipeline_image_barrier(
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            final_layout,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            desc_.usage & TextureUsage::Storage ? (VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT) : VK_ACCESS_2_SHADER_READ_BIT,
            aspect_mask,
            0,
            1,
            layer,
            1);

        if (!cmd_pool->end_single_time_commands(cmd_buffer))
        {
            Log::error("Failed to end single-time commands for texture upload");
            return false;
        }

        layer_layouts_[layer] = final_layout;

        return true;
    }

    void Texture::read_back(void* data, const std::size_t size, const std::uint32_t layer)
    {
        // Log::trace("Downloading {} bytes from texture", size);

        const std::unique_ptr<rhi::Buffer> staging_buffer = stage(size, layer);
        if (!staging_buffer)
        {
            return;
        }

        staging_buffer->read_back(data, size, 0);
    }

    void Texture::transition_layout(const TextureLayout old_layout, const TextureLayout new_layout)
    {
        transition_layout_internal(to_vk_image_layout(old_layout), to_vk_image_layout(new_layout));
    }

    VkImageAspectFlags Texture::aspect_mask() const
    {
        return image_aspect_flags_for_desc(desc_);
    }

    std::uint32_t Texture::layer_count() const
    {
        const bool is_cube =
            desc_.type == TextureType::TextureCube ||
            desc_.type == TextureType::TextureCubeArray;

        return desc_.array_layers * (is_cube ? 6u : 1u);
    }

    VkImage     Texture::vk_image() const { return image_; }
    VkImageView Texture::vk_image_view() const { return image_view_; }

    VkImageLayout Texture::vk_layout(const std::uint32_t layer) const
    {
        if (layer_layouts_.empty()) return VK_IMAGE_LAYOUT_UNDEFINED;
        if (layer >= layer_layouts_.size()) return layer_layouts_.back();
        return layer_layouts_[layer];
    }

    VkImageView Texture::vk_layer_image_view(const std::uint32_t layer) const
    {
        if (layer_image_views_.empty()) return image_view_;
        if (layer >= layer_image_views_.size())
        {
            Log::warn(
                "Texture layer image view {} out of bounds ({} layers)",
                layer,
                layer_image_views_.size());
            return image_view_;
        }

        return layer_image_views_[layer];
    }
}
