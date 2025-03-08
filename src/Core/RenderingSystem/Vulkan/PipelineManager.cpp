#include "PipelineManager.hpp"

#include "CommandPool.hpp"
#include "Device.hpp"

namespace boza
{
    void PipelineManager::cleanup()
    {
        auto& inst = instance();
        for (const auto& shader_module : inst.shader_modules | std::views::values)
            vkDestroyShaderModule(Device::get_device(), shader_module, nullptr);

        inst.pipelines.clear();
    }

    void PipelineManager::bind_pipeline(const VkCommandBuffer command_buffer, const pipeline_id_t id)
    {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, get_pipeline(id).get_pipeline());
    }

    Pipeline& PipelineManager::get_pipeline(const pipeline_id_t id)
    {
        assert(id != INVALID_PIPELINE_ID && "Invalid pipeline id");

        auto& inst = instance();
        assert(inst.pipelines.contains(id) && "Pipeline not found");
        return inst.pipelines.at(id);
    }

    void PipelineManager::destroy_pipeline(const pipeline_id_t id)
    {
        assert(id != INVALID_PIPELINE_ID && "Invalid pipeline id");

        auto& inst = instance();
        assert(inst.pipelines.contains(id) && "Pipeline not found");
        inst.pipelines.erase(id);
    }
}
