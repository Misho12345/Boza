export module boza.rhi.vulkan:descriptor;

import std;
import boza.rhi.objects;
import boza.common;

import <vk_all.h>;

export namespace boza::rhi::vk
{
    class DescriptorSetLayout final : public rhi::DescriptorSetLayout
    {
    public:
        bool init() override;
        void destroy() override;

        [[nodiscard]] VkDescriptorSetLayout vk_descriptor_set_layout() const;

    private:
        explicit DescriptorSetLayout(const DescriptorSetLayoutDesc& desc) : rhi::DescriptorSetLayout(desc) {}
        VkDescriptorSetLayout vk_descriptor_set_layout_{ nullptr };

        friend GraphicsObject;
    };


    class DescriptorPool final : public rhi::DescriptorPool
    {
    public:
        bool init() override;
        void destroy() override;

        DescriptorSet*              allocate_descriptor_set(rhi::DescriptorSetLayout* layout) override;
        std::vector<DescriptorSet*> allocate_descriptor_sets(
            uint32_t                                      count,
            const std::vector<rhi::DescriptorSetLayout*>& layouts) override;

        void free_descriptor_set(DescriptorSet* set) override;
        void free_descriptor_sets(const std::vector<DescriptorSet*>& sets) override;

        void recycle_descriptor_set(DescriptorSet* set) override;

        bool reset() override;

        [[nodiscard]] VkDescriptorPool vk_descriptor_pool() const;

    private:
        explicit         DescriptorPool(const DescriptorPoolDesc& desc) : rhi::DescriptorPool(desc) {}
        VkDescriptorPool vk_descriptor_pool_{ nullptr };

        friend GraphicsObject;
    };


    class DescriptorSet final : public rhi::DescriptorSet
    {
    public:
        ~DescriptorSet() override { destroy(); }

        bool init() override;
        void destroy() override;

        void update(const std::vector<DescriptorWrite>& writes) override;

        [[nodiscard]] VkDescriptorSet vk_descriptor_set() const;

    private:
        explicit DescriptorSet(const DescriptorSetDesc& desc) : rhi::DescriptorSet(desc) {}

        void            set_vk_descriptor_set(VkDescriptorSet set);
        VkDescriptorSet vk_descriptor_set_{ nullptr };

        friend DescriptorPool;
    };
}

namespace boza::rhi::vk
{
    VkDescriptorType to_vk(const DescriptorType type)
    {
        switch (type)
        {
            case DescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
            case DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case DescriptorType::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case DescriptorType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case DescriptorType::UniformTexelBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case DescriptorType::StorageTexelBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            case DescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case DescriptorType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case DescriptorType::UniformBufferDynamic: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case DescriptorType::StorageBufferDynamic: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case DescriptorType::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        }

        std::unreachable();
    }

    VkShaderStageFlags to_vk(const Flags<ShaderStage> stages)
    {
        VkShaderStageFlags result = 0;

        if (stages.has(ShaderStage::Vertex)) result |= VK_SHADER_STAGE_VERTEX_BIT;
        if (stages.has(ShaderStage::Fragment)) result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (stages.has(ShaderStage::Compute)) result |= VK_SHADER_STAGE_COMPUTE_BIT;
        if (stages.has(ShaderStage::TessControl)) result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (stages.has(ShaderStage::TessEvaluation)) result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (stages.has(ShaderStage::Geometry)) result |= VK_SHADER_STAGE_GEOMETRY_BIT;
        if (stages.has(ShaderStage::All)) result |= VK_SHADER_STAGE_ALL;

        return result;
    }
}