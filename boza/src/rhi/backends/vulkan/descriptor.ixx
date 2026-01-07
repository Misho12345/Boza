export module boza.rhi.vulkan:descriptor;

import std;
import boza.rhi.objects;
import boza.common;

import <vk_all>;

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
            uint32_t                             count,
            std::span<rhi::DescriptorSetLayout*> layouts) override;

        void free_descriptor_set(DescriptorSet* set) override;
        void free_descriptor_sets(std::span<DescriptorSet*> sets) override;

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

        void update(std::span<DescriptorWrite> writes) override;

        [[nodiscard]] VkDescriptorSet vk_descriptor_set() const;

    private:
        explicit DescriptorSet(const DescriptorSetDesc& desc) : rhi::DescriptorSet(desc) {}

        void            set_vk_descriptor_set(VkDescriptorSet set);
        VkDescriptorSet vk_descriptor_set_{ nullptr };

        friend DescriptorPool;
    };
}
