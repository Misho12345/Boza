module boza.rhi.vulkan;

import :resources;
import :util;

namespace boza::rhi::vk
{
    bool Sampler::init()
    {
        const auto vk_device_ptr = reinterpret_cast<Device*>(desc_.device);
        const auto vk_device = vk_device_ptr->logical_device();

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(vk_device_ptr->physical_device(), &properties);

        const bool requested_anisotropy = desc_.filter == SamplerFilter::Anisotropic;
        const bool anisotropy_enabled = requested_anisotropy && vk_device_ptr->sampler_anisotropy_enabled();

        if (requested_anisotropy && !anisotropy_enabled)
        {
            Log::warn("Sampler requested anisotropy but device feature is unavailable; falling back to linear filtering");
        }

        const float max_anisotropy = anisotropy_enabled
            ? std::clamp(desc_.max_anisotropy, 1.0f, properties.limits.maxSamplerAnisotropy)
            : 1.0f;

        const VkSamplerCreateInfo sampler_create_info
        {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = to_vk(desc_.filter),
            .minFilter = to_vk(desc_.filter),
            .mipmapMode = get_mipmap_mode(desc_.mipmap_mode),
            .addressModeU = to_vk(desc_.wrap_u),
            .addressModeV = to_vk(desc_.wrap_v),
            .addressModeW = to_vk(desc_.wrap_w),
            .mipLodBias = desc_.mip_lod_bias,
            .anisotropyEnable = anisotropy_enabled,
            .maxAnisotropy = max_anisotropy,
            .compareEnable = desc_.compare_enable,
            .compareOp = to_vk(desc_.compare_op),
            .minLod = desc_.min_lod,
            .maxLod = desc_.max_lod,
            .borderColor = to_vk(desc_.border_color),
            .unnormalizedCoordinates = desc_.unnormalized_coordinates
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
            const auto vk_device = reinterpret_cast<Device*>(desc_.device)->logical_device();
            vkDestroySampler(vk_device, sampler_, nullptr);
            sampler_ = nullptr;
        }
    }

    VkSampler Sampler::vk_sampler() const { return sampler_; }
}
