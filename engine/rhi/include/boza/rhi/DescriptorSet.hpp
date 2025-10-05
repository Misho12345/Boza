#pragma once
#include <variant>

#include "boza/std_pch.hpp"
#include "GraphicsObject.hpp"

#include "Buffer.hpp"
#include "Sampler.hpp"
#include "Texture.hpp"
#include "ShaderModule.hpp"

#include "boza/util/Flags.hpp"

namespace boza::rhi
{
    /// ---------------------------------
    /// ===== Descriptor Set Layout =====
    /// ---------------------------------

    struct DescriptorSetLayoutBinding
    {
        uint32_t           binding;
        Flags<ShaderStage> stages;
        uint32_t           count;
    };

    struct DescriptorSetLayoutDesc
    {
        Device*                                 device;
        std::vector<DescriptorSetLayoutBinding> bindings;
    };

    class DescriptorSetLayout : public GraphicsObject<DescriptorSetLayout, DescriptorSetLayoutDesc>
    {
    protected:
        explicit DescriptorSetLayout(const DescriptorSetLayoutDesc& desc) : GraphicsObject(desc) {}
    };

    /// ----------------------
    /// ===== Descriptor =====
    /// ----------------------

    struct UniformBuffer
    {
        Buffer*  buffer;
        uint32_t offset;
        uint32_t range;
    };

    struct CombinedImageSampler
    {
        Texture* texture;
        Sampler* sampler;
    };

    struct Descriptor
    {
        std::variant<UniformBuffer, CombinedImageSampler> data;
    };


    /// --------------------------
    /// ===== Descriptor Set =====
    /// --------------------------

    struct DescriptorSetDesc
    {
        Device*                 device;
        DescriptorSetLayout*    layout;
        std::vector<Descriptor> descriptors;
    };

    class DescriptorSet : public GraphicsObject<DescriptorSet, DescriptorSetDesc>
    {
    protected:
        explicit DescriptorSet(const DescriptorSetDesc& desc) : GraphicsObject(desc) {}
    };
}
