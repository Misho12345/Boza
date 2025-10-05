#pragma once
#include "pch.hpp"
#include "boza/rhi/Descriptor.hpp"

namespace boza::rhi::vk
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

        rhi::DescriptorSet*              allocate_descriptor_set(rhi::DescriptorSetLayout* layout) override;
        std::vector<rhi::DescriptorSet*> allocate_descriptor_sets(
            uint32_t                                      count,
            const std::vector<rhi::DescriptorSetLayout*>& layouts) override;

        void free_descriptor_set(rhi::DescriptorSet* set) override;
        void free_descriptor_sets(const std::vector<rhi::DescriptorSet*>& sets) override;

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
