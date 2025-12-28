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

    VkSamplerAddressMode to_vk(const SamplerWrap mode)
    {
        switch (mode)
        {
            case SamplerWrap::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case SamplerWrap::ClampToEdge: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case SamplerWrap::ClampToBorder: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case SamplerWrap::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        }

        std::unreachable();
    }

    VkSamplerMipmapMode get_mipmap_mode(const SamplerFilter mode)
    {
        switch (mode)
        {
            case SamplerFilter::Nearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            case SamplerFilter::Linear: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            case SamplerFilter::Anisotropic: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }

        std::unreachable();
    }

    VkBorderColor to_vk(const BorderColor color)
    {
        switch (color)
        {
            case BorderColor::FloatTransparentBlack: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            case BorderColor::IntTransparentBlack: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
            case BorderColor::FloatOpaqueBlack: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            case BorderColor::IntOpaqueBlack: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            case BorderColor::FloatOpaqueWhite: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            case BorderColor::IntOpaqueWhite: return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
        }

        std::unreachable();
    }

    VkCompareOp to_vk(const SamplerCompareOp op)
    {
        switch (op)
        {
            case SamplerCompareOp::Never: return VK_COMPARE_OP_NEVER;
            case SamplerCompareOp::Less: return VK_COMPARE_OP_LESS;
            case SamplerCompareOp::Equal: return VK_COMPARE_OP_EQUAL;
            case SamplerCompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
            case SamplerCompareOp::Greater: return VK_COMPARE_OP_GREATER;
            case SamplerCompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
            case SamplerCompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case SamplerCompareOp::Always: return VK_COMPARE_OP_ALWAYS;
        }

        std::unreachable();
    }

    bool Sampler::init()
    {
        const auto vk_device_ptr = reinterpret_cast<Device*>(desc_.device);
        const auto vk_device = vk_device_ptr->logical_device();

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(vk_device_ptr->physical_device(), &properties);

        const bool anisotropy_enabled = desc_.filter == SamplerFilter::Anisotropic;
        const float max_anisotropy = anisotropy_enabled
            ? std::min(desc_.max_anisotropy, properties.limits.maxSamplerAnisotropy)
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
            .unnormalizedCoordinates = desc_.unnormalized_coordinates,
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