#include "ShaderReflector.hpp"

namespace sp
{
    json ShaderReflector::generate_metadata(const std::vector<uint32_t>& spirv)
    {
        json metadata;

        const spirv_cross::Compiler        compiler{ spirv };
        const spirv_cross::ShaderResources resources{ compiler.get_shader_resources() };

        reflect_resources(compiler, resources.uniform_buffers, "uniform_buffers", metadata);
        reflect_resources(compiler, resources.storage_buffers, "storage_buffers", metadata);
        reflect_resources(compiler, resources.stage_inputs, "stage_inputs", metadata);
        reflect_resources(compiler, resources.stage_outputs, "stage_outputs", metadata);
        reflect_resources(compiler, resources.sampled_images, "sampled_images", metadata);
        reflect_resources(compiler, resources.storage_images, "storage_images", metadata);
        reflect_push_constants(compiler, resources, metadata);

        return metadata;
    }

    void ShaderReflector::reflect_resources(
        const spirv_cross::Compiler&                           compiler,
        const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
        const std::string&                                     type_name,
        json&                                                  metadata)
    {
        if (resources.empty()) return;
        json j_resources = json::array();

        for (const auto& resource : resources)
        {
            json j_resource;
            j_resource["name"] = compiler.get_name(resource.id);

            if (type_name != "stage_inputs" && type_name != "stage_outputs")
            {
                j_resource["set"]     = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                j_resource["binding"] = compiler.get_decoration(resource.id, spv::DecorationBinding);
            }
            else j_resource["location"] = compiler.get_decoration(resource.id, spv::DecorationLocation);

            j_resources.push_back(j_resource);
        }

        metadata[type_name] = j_resources;
    }

    void ShaderReflector::reflect_push_constants(
        const spirv_cross::Compiler&        compiler,
        const spirv_cross::ShaderResources& resources,
        json&                               metadata)
    {
        if (resources.push_constant_buffers.empty()) return;
        json push_constants_array = json::array();

        for (const auto& push_constant_resource : resources.push_constant_buffers)
        {
            json        j_pc;
            const auto& type = compiler.get_type(push_constant_resource.type_id);

            j_pc["name"] = compiler.get_name(push_constant_resource.id);

            if (auto ranges = compiler.get_active_buffer_ranges(push_constant_resource.id); !ranges.empty())
            {
                j_pc["offset"] = ranges[0].offset;
                j_pc["size"]   = ranges[0].range;
            }
            else
            {
                j_pc["offset"] = 0;
                j_pc["size"]   = compiler.get_declared_struct_size(type);
            }

            json members = json::array();

            for (uint32_t i = 0; i < type.member_types.size(); ++i)
            {
                json member;
                member["name"]   = compiler.get_member_name(type.self, i);
                member["offset"] = compiler.get_member_decoration(type.self, i, spv::DecorationOffset);

                member["size"] = compiler.get_declared_struct_member_size(type, i);
                members.push_back(member);
            }

            j_pc["members"] = members;
            push_constants_array.push_back(j_pc);
        }

        metadata["push_constants"] = push_constants_array;
    }
}
