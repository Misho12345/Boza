#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    class Pipeline final : public Singleton<Pipeline>
    {
    public:
        [[nodiscard]]
        static bool create();
        static void destroy();

        [[nodiscard]] static VkPipeline& get_pipeline();

    private:
        [[nodiscard]] bool create_pipeline();
        [[nodiscard]] bool create_pipeline_layout();

        VkPipeline       pipeline{ nullptr };
        VkPipelineLayout pipeline_layout{ nullptr };

        VkShaderModule vertex_module{ nullptr };
        VkShaderModule fragment_module{ nullptr };
    };
}
