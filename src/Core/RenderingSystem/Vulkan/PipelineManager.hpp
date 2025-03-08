#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"
#include "Pipeline.hpp"

namespace boza
{
    using pipeline_id = uint32_t;

    class PipelineManager final : public Singleton<PipelineManager>
    {
    public:
        static void cleanup();

        template<typename Vertex>
            requires requires {
                { Vertex::get_binding_description() } -> std::same_as<VkVertexInputBindingDescription>;
                { Vertex::get_attribute_descriptions() } -> std::same_as<std::vector<VkVertexInputAttributeDescription>>;
            }
        static pipeline_id create_pipeline(
            const std::string& vertex_shader,
            const std::string& fragment_shader,
            std::vector<DescriptorSetLayout> descriptor_set_layouts,
            std::vector<VkPushConstantRange> push_constant_ranges,
            VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL);

        static void      bind_pipeline(VkCommandBuffer command_buffer, pipeline_id id);
        static Pipeline& get_pipeline(pipeline_id id);
        static void      destroy_pipeline(pipeline_id id);

    private:
        hash_map<pipeline_id, Pipeline>       pipelines;
        hash_map<std::string, VkShaderModule> shader_modules;
        pipeline_id                           next_id{ 0 };
    };
}

#include "PipelineManager.inl"