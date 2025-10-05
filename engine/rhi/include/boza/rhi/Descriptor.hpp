#pragma once
#include <variant>

#include "boza/std_pch.hpp"
#include "GraphicsObject.hpp"

#include "Resources.hpp"
#include "ShaderModule.hpp"

#include "boza/util/Flags.hpp"

namespace boza::rhi
{
    class DescriptorSet;
    class DescriptorSetLayout;

    /// ---------------------------------
    /// ===== Descriptor Set Layout =====
    /// ---------------------------------

    enum class DescriptorType : uint8_t
    {
        Sampler,
        CombinedImageSampler,
        SampledImage,
        StorageImage,
        UniformTexelBuffer,
        StorageTexelBuffer,
        UniformBuffer,
        StorageBuffer,
        UniformBufferDynamic,
        StorageBufferDynamic,
        InputAttachment
    };

    struct DescriptorSetLayoutBinding
    {
        uint32_t           binding;
        DescriptorType     type;
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

    /// ---------------------------
    /// ===== Descriptor Pool =====
    /// ---------------------------

    struct DescriptorPoolSize
    {
        DescriptorType type;
        uint32_t       count;
    };

    struct DescriptorPoolDesc
    {
        Device*  device;
        uint32_t max_sets;

        std::vector<DescriptorPoolSize> pool_sizes;
    };

    class DescriptorPool : public GraphicsObject<DescriptorPool, DescriptorPoolDesc>
    {
    public:
        virtual DescriptorSet*              allocate_descriptor_set(DescriptorSetLayout* layout) = 0;
        virtual std::vector<DescriptorSet*> allocate_descriptor_sets(
            uint32_t                                 count,
            const std::vector<DescriptorSetLayout*>& layouts) = 0;

        virtual void free_descriptor_set(DescriptorSet* set) = 0;
        virtual void free_descriptor_sets(const std::vector<DescriptorSet*>& sets) = 0;

        virtual bool reset() = 0;

    protected:
        explicit DescriptorPool(const DescriptorPoolDesc& desc) : GraphicsObject(desc) {}
    };


    /// --------------------------
    /// ===== Descriptor Set =====
    /// --------------------------

    struct UniformBuffer
    {
        Buffer*  buffer;
        uint32_t offset;
        uint32_t range;
    };

    struct StorageBuffer
    {
        Buffer*  buffer;
        uint32_t offset;
        uint32_t range;
    };

    struct CombinedImageSampler
    {
        Sampler* sampler;
        Texture* texture;
    };

    using DescriptorInfo = std::variant<UniformBuffer, StorageBuffer, CombinedImageSampler>;

    struct DescriptorWrite
    {
        uint32_t       binding;
        uint32_t       array_element;
        DescriptorType type;
        DescriptorInfo info;
    };

    struct DescriptorSetDesc
    {
        Device*              device;
        DescriptorPool*      pool;
        DescriptorSetLayout* layout;
    };

    class DescriptorSet
    {
    public:
        virtual ~DescriptorSet() = default;

        virtual bool init() = 0;
        virtual void destroy() = 0;

        virtual void update(const std::vector<DescriptorWrite>& writes) = 0;

    protected:
        explicit          DescriptorSet(const DescriptorSetDesc& desc) : desc(desc) {}
        DescriptorSetDesc desc;
    };
}
