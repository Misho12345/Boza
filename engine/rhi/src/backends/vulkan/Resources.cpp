#include "Resources.hpp"
#include "Allocator.hpp"
#include "Command.hpp"
#include "Device.hpp"
#include "boza/core/Logger.hpp"

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
                case SamplerFilter::Anisotropic: return VK_FILTER_LINEAR;
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
        // Logger::trace("Creating buffer ({} bytes)", desc.size);

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
        // Logger::trace("Destroying buffer");

        if (buffer_)
        {
            const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();
            vmaDestroyBuffer(allocator, buffer_, allocation_);
        }
    }


    void* Buffer::map()
    {
        // Logger::trace("Mapping buffer memory");

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
        // Logger::trace("Unmapping buffer memory");

        const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();
        vmaUnmapMemory(allocator, allocation_);
    }

    size_t Buffer::size() const { return desc.size; }

    void Buffer::upload(const void* data, const size_t size, const size_t offset)
    {
        // Logger::trace("Uploading {} bytes to buffer at offset {}", size, offset);

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
        // Logger::trace("Creating texture ({}x{})", desc.width, desc.height);

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
        // Logger::trace("Destroying texture");

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
        // Logger::trace("Transitioning texture layout: {} -> {}", static_cast<uint32_t>(old_layout), static_cast<uint32_t>(new_layout));

        const auto* device = reinterpret_cast<Device*>(desc.device);
        auto* cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);

        auto* cmd_buffer = cmd_pool->begin_single_time_commands();
        if (!cmd_buffer)
        {
            Logger::error("Failed to begin single-time commands for texture layout transition");
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
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
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
        // Logger::trace("Uploading {} bytes to texture", size);

        const auto* device = reinterpret_cast<Device*>(desc.device);
        const auto allocator = device->allocator()->vma_allocator();

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

        VkBuffer staging_buffer;
        VmaAllocation staging_allocation;

        VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &staging_buffer, &staging_allocation, nullptr),
        {
            LOG_VK_ERROR("Failed to create staging buffer for texture upload");
            return;
        });

        void* mapped_data;
        vmaMapMemory(allocator, staging_allocation, &mapped_data);
        memcpy(mapped_data, data, size);
        vmaUnmapMemory(allocator, staging_allocation);

        transition_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        auto* cmd_pool = device->command_pool(device->queue_family_indices().graphics_family);
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
        // Logger::trace("Loading texture from file: {}", filepath);

        int width, height, channels;

        if (!stbi_info(filepath.c_str(), &width, &height, &channels))
        {
            Logger::error("Failed to query texture info from file: {}", filepath);
            return false;
        }

        int desired_channels = 4; // Default to RGBA
        switch (desc.format)
        {
            case TextureFormat::R8:
            case TextureFormat::R16F:
            case TextureFormat::R32F:
                desired_channels = 1;
                break;
            case TextureFormat::RG8:
            case TextureFormat::RG16F:
            case TextureFormat::RG32F:
                desired_channels = 2;
                break;
            case TextureFormat::RGB8:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB32F:
                desired_channels = 3;
                break;
            case TextureFormat::RGBA8:
            case TextureFormat::BGRA8:
            case TextureFormat::RGBA16F:
            case TextureFormat::RGBA32F:
                desired_channels = 4;
                break;
            case TextureFormat::DEPTH24STENCIL8:
            case TextureFormat::DEPTH32F:
                Logger::warn("Cannot load depth/stencil texture from image file: {}", filepath);
                return false;
        }

        const int load_channels = std::max(channels, desired_channels);

        unsigned char* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, load_channels);

        if (!pixels)
        {
            Logger::error("Failed to load texture from file: {}", filepath);
            return false;
        }

        desc.width = width;
        desc.height = height;

        if (image_)
        {
            const auto device = reinterpret_cast<Device*>(desc.device)->logical_device();
            const auto allocator = reinterpret_cast<Device*>(desc.device)->allocator()->vma_allocator();

            if (image_view_)
            {
                vkDestroyImageView(device, image_view_, nullptr);
                image_view_ = VK_NULL_HANDLE;
            }

            vmaDestroyImage(allocator, image_, allocation_);
            image_ = VK_NULL_HANDLE;
            allocation_ = VK_NULL_HANDLE;
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

        const size_t image_size = width * height * load_channels;
        upload(pixels, image_size);

        stbi_image_free(pixels);

        // Logger::trace("Loaded texture from file: {} ({}x{}, {} channels)", filepath, width, height, load_channels);
        return true;
    }

    VkImage     Texture::vk_image() const { return image_; }
    VkImageView Texture::vk_image_view() const { return image_view_; }

    /// -------------------
    /// ===== Sampler =====
    /// -------------------

    bool Sampler::init()
    {
        // Logger::trace("Creating sampler");

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
        // Logger::trace("Destroying sampler");

        if (sampler_)
        {
            const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
            vkDestroySampler(vk_device, sampler_, nullptr);
            sampler_ = nullptr;
        }
    }

    VkSampler Sampler::vk_sampler() const { return sampler_; }
}
