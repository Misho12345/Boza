#pragma once
#include "boza_pch.hpp"
#include "Memory/DescriptorSetLayout.hpp"

namespace boza
{
    struct PipelineCreateInfo
    {
        VkVertexInputBindingDescription binding_description;
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
        std::vector<DescriptorSetLayout> descriptor_set_layouts;
        std::vector<VkPushConstantRange> push_constant_ranges;

        VkShaderModule vertex_shader{ nullptr };
        VkShaderModule fragment_shader{ nullptr };

        VkPolygonMode polygon_mode{ VK_POLYGON_MODE_FILL };
    };

    class Pipeline final
    {
    public:
        explicit Pipeline(const PipelineCreateInfo& create_info);
        ~Pipeline();

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        Pipeline(Pipeline&& other) noexcept;
        Pipeline& operator=(Pipeline&& other) noexcept;

        VkPipeline& get_pipeline();
        VkPipelineLayout& get_layout();

    private:
        bool create_pipeline(const PipelineCreateInfo& create_info);
        bool create_pipeline_layout(const PipelineCreateInfo& create_info);

        VkPipeline pipeline{ nullptr };
        VkPipelineLayout layout{ nullptr };
    };
}
