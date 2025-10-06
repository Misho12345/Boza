#include "Resources.hpp"
#include "Allocator.hpp"
#include "Command.hpp"
#include "Device.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace boza::rhi::vk
{
    namespace
    {
        VkBufferUsageFlags to_vk(const BufferUsage usage)
        {
            switch (usage)
            {
                case BufferUsage::Vertex: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                case BufferUsage::Index: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                case BufferUsage::Uniform: return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                case BufferUsage::Storage: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }

            std::unreachable();
        }

        VkFormat to_vk(const TextureFormat format)
        {
            switch (format)
            {
                case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
                case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
                case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
                case TextureFormat::DEPTH24STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
                case TextureFormat::DEPTH32F: return VK_FORMAT_D32_SFLOAT;
            }

            std::unreachable();
        }

        VkImageUsageFlags to_vk(const TextureUsage usage)
        {
            switch (usage)
            {
                case TextureUsage::Sampled: return VK_IMAGE_USAGE_SAMPLED_BIT;
                case TextureUsage::Storage: return VK_IMAGE_USAGE_STORAGE_BIT;
                case TextureUsage::ColorAttachment: return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                case TextureUsage::DepthStencilAttachment: return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                case TextureUsage::TransferSrc: return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                case TextureUsage::TransferDst: return VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                case TextureUsage::InputAttachment: return VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            }

            std::unreachable();
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
                case TextureUsage::InputAttachment:
                    return VK_IMAGE_ASPECT_COLOR_BIT;

                case TextureUsage::DepthStencilAttachment:
                    return VK_IMAGE_ASPECT_DEPTH_BIT;
            }

            std::unreachable();
        }

        VmaMemoryUsage to_vma(const BufferMemoryType memory_type)
        {
            switch (memory_type)
            {
                case BufferMemoryType::DeviceLocal: return VMA_MEMORY_USAGE_GPU_ONLY;
                case BufferMemoryType::HostVisible: return VMA_MEMORY_USAGE_CPU_TO_GPU;
                case BufferMemoryType::HostCoherent: return VMA_MEMORY_USAGE_CPU_ONLY;
            }

            std::unreachable();
        }

        VkFilter to_vk(const SamplerFilter filter)
        {
            switch (filter)
            {
                case SamplerFilter::Nearest: return VK_FILTER_NEAREST;
                case SamplerFilter::Linear: return VK_FILTER_LINEAR;
                case SamplerFilter::Anisotropic: return VK_FILTER_LINEAR; // Anisotropy is enabled separately
            }
            std::unreachable();
        }

        VkSamplerAddressMode to_vk(const SamplerAddressMode mode)
        {
            switch (mode)
            {
                case SamplerAddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
                case SamplerAddressMode::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                case SamplerAddressMode::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            }
            std::unreachable();
        }
    }

    /// ------------------
    /// ===== Buffer =====
    /// ------------------

    bool Buffer::init()
    {
        const VkBufferCreateInfo buffer_create_info
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = desc.size,
            .usage = to_vk(desc.usage),
        };

        VmaAllocationCreateInfo allocation_create_info{};
        allocation_create_info.usage = to_vma(desc.memory_type);

        const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();

        VK_CHECK(vmaCreateBuffer(
            allocator, &buffer_create_info, &allocation_create_info,
            &buffer_, &allocation_, &allocation_info_),
        {
            LOG_VK_ERROR("Failed to create buffer");
            return false;
        });

        return true;
    }

    void Buffer::destroy()
    {
        if (buffer_)
        {
            const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();
            vmaDestroyBuffer(allocator, buffer_, allocation_);
        }
    }


    void* Buffer::map()
    {
        void* mapped_data;
        const auto  allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();

        VK_CHECK(vmaMapMemory(allocator, allocation_, &mapped_data),
        {
            LOG_VK_ERROR("Failed to map buffer memory");
            return nullptr;
        });

        return mapped_data;
    }

    void Buffer::unmap()
    {
        const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();
        vmaUnmapMemory(allocator, allocation_);
    }

    size_t Buffer::size() const { return desc.size; }

    void Buffer::upload(const void* data, const size_t size, const size_t offset)
    {
        assert(desc.memory_type != BufferMemoryType::DeviceLocal && "Cannot upload to device local buffer");
        assert(offset + size <= desc.size && "Upload out of bounds");

        if (const auto mapped_data = map())
        {
            memcpy(static_cast<char*>(mapped_data) + offset, data, size);
            unmap();
        }
    }

    VkBuffer Buffer::vk_buffer() const { return buffer_; }

    /// -------------------
    /// ===== Texture =====
    /// -------------------

    bool Texture::init()
    {
        // Don't create the image here if width and height are placeholder values
        // The image will be created in load_from_file or upload
        if (desc.width == 0 || desc.height == 0)
        {
            Logger::warn("Texture created with zero dimensions, deferring initialization");
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

        VK_CHECK(vmaCreateImage(
            allocator, &image_create_info, &allocation_create_info,
            &image_, &allocation_, &allocation_info_),
        {
            LOG_VK_ERROR("Failed to create image");
            return false;
        });

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
        VK_CHECK(vkCreateImageView(vk_device, &image_view_create_info, nullptr, &image_view_),
        {
            LOG_VK_ERROR("Failed to create image view");
            return false;
        });

        return true;
    }

    void Texture::destroy()
    {
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

    void Texture::transition_layout(VkImageLayout old_layout, VkImageLayout new_layout)
    {
        auto* device_ptr = reinterpret_cast<Device*>(desc.device);
        auto* cmd_pool = device_ptr->command_pool(device_ptr->queue_family_indices().graphics_family);

        auto* cmd_buffer = cmd_pool->begin_single_time_commands();
        if (!cmd_buffer)
        {
            Logger::error("Failed to begin single-time commands for texture layout transition");
            return;
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image_;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            Logger::error("Unsupported layout transition");
            return;
        }

        auto* vk_cmd_buffer = reinterpret_cast<vk::CommandBuffer*>(cmd_buffer);

        vkCmdPipelineBarrier(
            vk_cmd_buffer->vk_command_buffer(),
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
        auto* device_ptr = reinterpret_cast<Device*>(desc.device);
        const auto allocator = device_ptr->allocator()->vma_allocator();

        // Create staging buffer
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo alloc_info{};
        alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        VkBuffer staging_buffer;
        VmaAllocation staging_allocation;

        VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &staging_buffer, &staging_allocation, nullptr),
        {
            LOG_VK_ERROR("Failed to create staging buffer for texture upload");
            return;
        });

        // Copy data to staging buffer
        void* mapped_data;
        vmaMapMemory(allocator, staging_allocation, &mapped_data);
        memcpy(mapped_data, data, size);
        vmaUnmapMemory(allocator, staging_allocation);

        // Transition image layout to transfer destination
        transition_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Copy buffer to image
        auto* cmd_pool = device_ptr->command_pool(device_ptr->queue_family_indices().graphics_family);
        auto* cmd_buffer = cmd_pool->begin_single_time_commands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {desc.width, desc.height, 1};

        auto* vk_cmd_buffer = reinterpret_cast<vk::CommandBuffer*>(cmd_buffer);
        vkCmdCopyBufferToImage(
            vk_cmd_buffer->vk_command_buffer(),
            staging_buffer,
            image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        cmd_pool->end_single_time_commands(cmd_buffer);

        // Transition to shader read-only optimal
        transition_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Cleanup staging buffer
        vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
    }

    bool Texture::load_from_file(const std::string& filepath)
    {
        int width, height, channels;
        unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels)
        {
            Logger::error("Failed to load texture from file: {}", filepath);
            return false;
        }

        // Update descriptor with actual image dimensions
        const_cast<TextureDesc&>(desc).width = width;
        const_cast<TextureDesc&>(desc).height = height;

        // Create the image now that we know the dimensions
        if (!image_)
        {
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

            VK_CHECK(vmaCreateImage(
                allocator, &image_create_info, &allocation_create_info,
                &image_, &allocation_, &allocation_info_),
            {
                LOG_VK_ERROR("Failed to create image");
                stbi_image_free(pixels);
                return false;
            });

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
            VK_CHECK(vkCreateImageView(vk_device, &image_view_create_info, nullptr, &image_view_),
            {
                LOG_VK_ERROR("Failed to create image view");
                stbi_image_free(pixels);
                return false;
            });
        }

        const size_t image_size = width * height * 4; // RGBA
        upload(pixels, image_size);

        stbi_image_free(pixels);

        Logger::trace("Loaded texture from file: {} ({}x{})", filepath, width, height);
        return true;
    }

    VkImage     Texture::vk_image() const { return image_; }
    VkImageView Texture::vk_image_view() const { return image_view_; }

    /// -------------------
    /// ===== Sampler =====
    /// -------------------

    bool Sampler::init()
    {
        const auto vk_device_ptr = reinterpret_cast<Device*>(desc.device);
        const auto vk_device = vk_device_ptr->logical_device();

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(vk_device_ptr->physical_device(), &properties);

        const VkSamplerCreateInfo sampler_create_info
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = to_vk(desc.filter),
            .minFilter = to_vk(desc.filter),
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = to_vk(desc.address_mode_u),
            .addressModeV = to_vk(desc.address_mode_v),
            .addressModeW = to_vk(desc.address_mode_w),
            .mipLodBias = 0.0f,
            .anisotropyEnable = (desc.filter == SamplerFilter::Anisotropic) ? VK_TRUE : VK_FALSE,
            .maxAnisotropy = (desc.filter == SamplerFilter::Anisotropic) ? properties.limits.maxSamplerAnisotropy : 1.0f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = VK_LOD_CLAMP_NONE,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        VK_CHECK(vkCreateSampler(vk_device, &sampler_create_info, nullptr, &sampler_),
        {
            LOG_VK_ERROR("Failed to create sampler");
            return false;
        });

        return true;
    }

    void Sampler::destroy()
    {
        if (sampler_)
        {
            const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
            vkDestroySampler(vk_device, sampler_, nullptr);
            sampler_ = nullptr;
        }
    }

    VkSampler Sampler::vk_sampler() const { return sampler_; }
}
