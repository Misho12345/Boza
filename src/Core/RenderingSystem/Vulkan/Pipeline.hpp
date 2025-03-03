#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    class Pipeline final : public Singleton<Pipeline>
    {
    public:
        struct PushConstant final
        {
            glm::vec4 colors[3];
        };

        [[nodiscard]]
        static bool create();
        static void destroy();

        [[nodiscard]] static VkPipeline& get_pipeline();
        [[nodiscard]] static VkPipelineLayout& get_pipeline_layout();

    private:
        [[nodiscard]] bool create_pipeline();
        [[nodiscard]] bool create_pipeline_layout();

        VkPipeline       pipeline{ nullptr };
        VkPipelineLayout pipeline_layout{ nullptr };

        VkShaderModule vertex_module{ nullptr };
        VkShaderModule fragment_module{ nullptr };
    };
}
