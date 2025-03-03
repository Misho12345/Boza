#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class DescriptorSetLayout final
    {
    public:
        static DescriptorSetLayout create_uniform_layout();
        static DescriptorSetLayout create_sampler_layout();

        void destroy() const;

        [[nodiscard]] VkDescriptorSetLayout& get_layout();

    private:
        VkDescriptorSetLayout layout{ nullptr };
    };
}
