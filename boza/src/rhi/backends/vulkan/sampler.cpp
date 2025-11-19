module boza.rhi.vulkan;

import :resources;
import :util;


namespace boza::rhi::vk
{
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

    bool Sampler::init()
    {
        // Log::trace("Creating sampler");

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
            .anisotropyEnable = desc.filter == SamplerFilter::Anisotropic,
            .maxAnisotropy = desc.filter == SamplerFilter::Anisotropic ? properties.limits.maxSamplerAnisotropy : 1.0f,
            .compareEnable = false,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = vk_log_clamp_none,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = false,
        };

        if (!vk_check(
            vkCreateSampler(vk_device, &sampler_create_info, nullptr, &sampler_),
            "Failed to create sampler"))
            return false;

        return true;
    }

    void Sampler::destroy()
    {
        // Log::trace("Destroying sampler");

        if (sampler_)
        {
            const auto vk_device = reinterpret_cast<Device*>(desc.device)->logical_device();
            vkDestroySampler(vk_device, sampler_, nullptr);
            sampler_ = nullptr;
        }
    }

    VkSampler Sampler::vk_sampler() const { return sampler_; }
}