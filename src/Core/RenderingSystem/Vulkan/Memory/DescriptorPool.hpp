#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    class DescriptorPool final : public Singleton<DescriptorPool>
    {
    public:
        [[nodiscard]]
        static bool create();
        static void destroy();

        [[nodiscard]] static VkDescriptorPool& get_descriptor_pool();

    private:
        VkDescriptorPool descriptor_pool{ nullptr };

        friend Singleton;
        DescriptorPool() = default;
    };
}
