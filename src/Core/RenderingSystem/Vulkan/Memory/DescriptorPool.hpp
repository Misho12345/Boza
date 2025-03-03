#pragma once
#include "boza_pch.hpp"
#include "DescriptorSetLayout.hpp"
#include "Singleton.hpp"

namespace boza
{
    class DescriptorPool final : public Singleton<DescriptorPool>
    {
    public:
        [[nodiscard]]
        static bool create();
        static void destroy();

        static VkDescriptorSet create_descriptor_set(DescriptorSetLayout& layout);

        [[nodiscard]] static VkDescriptorPool& get_descriptor_pool();

    private:
        VkDescriptorPool descriptor_pool{ nullptr };
    };
}
