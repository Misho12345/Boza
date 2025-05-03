#include "Texture.hpp"

#include "Allocator.hpp"
#include "GPU/Vulkan/Core/Device.hpp"
#include "GPU/Vulkan/Core/CommandPool.hpp"
#include "GPU/Vulkan/Descriptor/DescriptorSet.hpp"
#include "Logger.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace boza
{
    Texture::Texture(Texture&& other) noexcept
    {
        image = std::exchange(other.image, nullptr);
        image_view = std::exchange(other.image_view, nullptr);
        sampler = std::exchange(other.sampler, nullptr);
        allocation = std::exchange(other.allocation, nullptr);
        width = std::exchange(other.width, 0);
        height = std::exchange(other.height, 0);
        format = std::exchange(other.format, VK_FORMAT_UNDEFINED);
    }

    Texture& Texture::operator=(Texture&& other) noexcept
    {
        if (this != &other)
        {
            destroy();

            image = std::exchange(other.image, nullptr);
            image_view = std::exchange(other.image_view, nullptr);
            sampler = std::exchange(other.sampler, nullptr);
            allocation = std::exchange(other.allocation, nullptr);
            width = std::exchange(other.width, 0);
            height = std::exchange(other.height, 0);
            format = std::exchange(other.format, VK_FORMAT_UNDEFINED);
        }
        return *this;
    }


    Texture Texture::create_from_file(const std::string& filepath)
    {
        int tex_width, tex_height, tex_channels;
        stbi_set_flip_vertically_on_load(true);
        stbi_uc* pixels = stbi_load(filepath.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);

        if (!pixels)
        {
            Logger::error("Failed to load texture '{}'", filepath);
            return {};
        }

        const VkDeviceSize image_size = tex_width * tex_height * 4;

        Buffer staging_buffer = Buffer::create_staging_buffer(image_size);
        staging_buffer.write(pixels, image_size, 0);

        stbi_image_free(pixels);

        Texture texture = create_empty(tex_width, tex_height, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        if (texture.image == nullptr)
        {
            staging_buffer.destroy();
            return {};
        }

        if (!texture.transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
        {
            staging_buffer.destroy();
            texture.destroy();
            return {};
        }

        if (!texture.copy_buffer_to_image(staging_buffer.get_buffer()))
        {
            staging_buffer.destroy();
            texture.destroy();
            return {};
        }

        if (!texture.transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
        {
            staging_buffer.destroy();
            texture.destroy();
            return {};
        }

        staging_buffer.destroy();

        if (!texture.create_image_view(VK_FORMAT_R8G8B8A8_UNORM) || !texture.create_sampler())
        {
            texture.destroy();
            return {};
        }

        return texture;
    }

    Texture Texture::create_empty(const uint32_t width, const uint32_t height, const VkFormat format, const VkImageUsageFlags usage)
    {
        Texture texture;

        texture.width = width;
        texture.height = height;
        texture.format = format;

        const VkImageCreateInfo image_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = { .width = width, .height = height, .depth = 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        constexpr VmaAllocationCreateInfo alloc_info
        {
            .flags = {},
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        };

        VK_CHECK(vmaCreateImage(Allocator::get_vma_allocator(), &image_info, &alloc_info,
                               &texture.image, &texture.allocation, nullptr),
        {
            LOG_VK_ERROR("Failed to create texture image");
            return {};
        });

        return texture;
    }


    bool Texture::create_image_view(const VkFormat format)
    {
        const VkImageViewCreateInfo view_info
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        VK_CHECK(vkCreateImageView(Device::get_device(), &view_info, nullptr, &image_view),
        {
            LOG_VK_ERROR("Failed to create texture image view");
            return false;
        });

        return true;
    }

    bool Texture::create_sampler()
    {
        constexpr VkSamplerCreateInfo sampler_info
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = {},
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias = 0.0f,
            .anisotropyEnable = VK_FALSE,
            .maxAnisotropy = 0.0f,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE
        };

        VK_CHECK(vkCreateSampler(Device::get_device(), &sampler_info, nullptr, &sampler),
        {
            LOG_VK_ERROR("Failed to create texture sampler");
            return false;
        });

        return true;
    }


    bool Texture::transition_image_layout(const VkImageLayout old_layout, const VkImageLayout new_layout) const
    {
        VkCommandBuffer command_buffer = CommandPool::begin_single_time_commands();

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
            .image = image,
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
        else
        {
            Logger::error("Unsupported layout transition!");
            return false;
        }

        vkCmdPipelineBarrier(
            command_buffer,
            source_stage, destination_stage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        return CommandPool::end_single_time_commands(command_buffer);
    }

    bool Texture::copy_buffer_to_image(VkBuffer buffer) const
    {
        VkCommandBuffer command_buffer = CommandPool::begin_single_time_commands();

        const VkBufferImageCopy region
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
            .imageOffset = { .x = 0, .y = 0, .z = 0 },
            .imageExtent = { .width = width, .height = height, .depth = 1 }
        };

        vkCmdCopyBufferToImage(
            command_buffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        return CommandPool::end_single_time_commands(command_buffer);
    }


    void Texture::destroy()
    {
        if (sampler != nullptr)
        {
            vkDestroySampler(Device::get_device(), sampler, nullptr);
            sampler = nullptr;
        }

        if (image_view != nullptr)
        {
            vkDestroyImageView(Device::get_device(), image_view, nullptr);
            image_view = nullptr;
        }

        if (image != nullptr)
        {
            vmaDestroyImage(Allocator::get_vma_allocator(), image, allocation);
            image = nullptr;
            allocation = nullptr;
        }
    }

    VkImageView& Texture::get_image_view() { return image_view; }
    VkSampler& Texture::get_sampler() { return sampler; }
    uint32_t Texture::get_width() const { return width; }
    uint32_t Texture::get_height() const { return height; }
    VkFormat Texture::get_format() const { return format; }
}