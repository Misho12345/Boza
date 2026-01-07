export module boza.rhi.objects:descriptor;

import std;
import boza.common;

import :graphics_object;
import :resources;
import :shader_module;

export namespace boza::rhi
{
    class DescriptorSet;
    class DescriptorSetLayout;

    /// ---------------------------------
    /// ===== Descriptor Set Layout =====
    /// ---------------------------------

    enum class DescriptorType : std::uint8_t
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
        std::uint32_t      binding;
        DescriptorType     type;
        Flags<ShaderStage> stages;
        std::uint32_t      count;
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
        std::uint32_t  count;
    };

    struct DescriptorPoolDesc
    {
        Device*       device;
        std::uint32_t max_sets;

        std::vector<DescriptorPoolSize> pool_sizes;
    };

    class DescriptorPool : public GraphicsObject<DescriptorPool, DescriptorPoolDesc>
    {
    public:
        virtual DescriptorSet*              allocate_descriptor_set(DescriptorSetLayout* layout) = 0;
        virtual std::vector<DescriptorSet*> allocate_descriptor_sets(
            std::uint32_t                   count,
            std::span<DescriptorSetLayout*> layouts) = 0;

        virtual void free_descriptor_set(DescriptorSet* set) = 0;
        virtual void free_descriptor_sets(std::span<DescriptorSet*> sets) = 0;

        virtual bool reset() = 0;

    protected:
        explicit DescriptorPool(const DescriptorPoolDesc& desc) : GraphicsObject(desc) {}

        flat_map<std::size_t, std::vector<DescriptorSet*>> free_sets_by_layout_;
        std::uint64_t current_generation_{ 0 };
    };


    /// --------------------------
    /// ===== Descriptor Set =====
    /// --------------------------

    struct UniformBuffer
    {
        Buffer*       buffer;
        std::uint32_t offset;
        std::uint32_t range;
    };

    struct StorageBuffer
    {
        Buffer*       buffer;
        std::uint32_t offset;
        std::uint32_t range;
    };

    struct CombinedImageSampler
    {
        Texture* texture;
        Sampler* sampler;
    };

    struct StorageImage
    {
        Texture* texture;
    };

    using DescriptorInfo = std::variant<UniformBuffer, StorageBuffer, CombinedImageSampler, StorageImage>;

    struct DescriptorWrite
    {
        std::uint32_t  binding;
        std::uint32_t  array_element;
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

        virtual void update(std::span<DescriptorWrite> writes) = 0;

        std::uint64_t generation{ 0 };

    protected:
        explicit DescriptorSet(const DescriptorSetDesc& desc) : desc_(desc) {}
        DescriptorSetDesc desc_;
    };
}
