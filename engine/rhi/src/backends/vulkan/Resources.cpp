#include "Resources.hpp"
#include "Allocator.hpp"
#include "Device.hpp"

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
            .usage = to_vk(desc.usage),
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
