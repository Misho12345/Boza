#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"
#include "Pipeline.hpp"

namespace boza
{
    using pipeline_id_t                         = uint32_t;
    constexpr pipeline_id_t INVALID_PIPELINE_ID = std::numeric_limits<pipeline_id_t>::max();

    class PipelineManager final : public Singleton<PipelineManager>
    {
    public:
        static void cleanup();

        static pipeline_id_t create_pipeline(
            const std::string&                 vertex_shader,
            const std::string&                 fragment_shader,
            VkPolygonMode                      polygon_mode = VK_POLYGON_MODE_FILL);

        static void      bind_pipeline(VkCommandBuffer command_buffer, pipeline_id_t id);
        static Pipeline& get_pipeline(pipeline_id_t id);
        static void      destroy_pipeline(pipeline_id_t id);

    private:
        hash_map<pipeline_id_t, Pipeline>     pipelines;
        hash_map<std::string, VkShaderModule> shader_modules;
        pipeline_id_t                         next_id{ 0 };

        friend Singleton;
        PipelineManager() = default;
    };
}
