#pragma once
#include "PipelineManager.hpp"
#include "ShaderLoader.hpp"
#include "Logger.hpp"
#include "GPU/Vulkan/Vertex/VertexLayout.hpp"

namespace boza
{
    template<typename Vertex>
    pipeline_id_t PipelineManager::create_pipeline(
        const std::string&                 vertex_shader,
        const std::string&                 fragment_shader,
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts,
        std::vector<VkPushConstantRange>   push_constant_ranges,
        const VkPolygonMode                polygon_mode)
    {
        if (vertex_shader.empty() || fragment_shader.empty())
        {
            Logger::error("Failed to create pipeline: shader path is empty");
            return INVALID_PIPELINE_ID;
        }

        const VertexLayout layout = get_layout<Vertex>();
        if (layout.attributes.size() == 0) return INVALID_PIPELINE_ID;

        auto& inst = instance();

        VkShaderModule vertex_shader_module;
        VkShaderModule fragment_shader_module;

        if (inst.shader_modules.contains(vertex_shader))
        {
            vertex_shader_module = inst.shader_modules.at(vertex_shader);
        }
        else if ((vertex_shader_module = ShaderLoader::create_shader_module(vertex_shader)) != nullptr)
        {
            inst.shader_modules.emplace(vertex_shader, vertex_shader_module);
        }
        else
        {
            Logger::error("Failed to create pipeline: vertex shader creation failed");
            return INVALID_PIPELINE_ID;
        }


        if (inst.shader_modules.contains(fragment_shader))
        {
            fragment_shader_module = inst.shader_modules.at(fragment_shader);
        }
        else if ((fragment_shader_module = ShaderLoader::create_shader_module(fragment_shader)) != nullptr)
        {
            inst.shader_modules.emplace(fragment_shader, fragment_shader_module);
        }
        else
        {
            Logger::error("Failed to create pipeline: fragment shader creation failed");
            return INVALID_PIPELINE_ID;
        }

        const PipelineCreateInfo create_info
        {
            .binding_description = layout.binding,
            .attribute_descriptions = std::move(layout.attributes),
            .descriptor_set_layouts = std::move(descriptor_set_layouts),
            .push_constant_ranges = std::move(push_constant_ranges),
            .vertex_shader = vertex_shader_module,
            .fragment_shader = fragment_shader_module,
            .polygon_mode = polygon_mode
        };

        Pipeline pipeline{ create_info };
        if (pipeline.get_pipeline() == nullptr ||
            pipeline.get_layout() == nullptr) return INVALID_PIPELINE_ID;
        inst.pipelines.emplace(inst.next_id, std::move(pipeline));
        return inst.next_id++;
    }
}
