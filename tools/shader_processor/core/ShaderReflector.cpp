#include "ShaderReflector.hpp"
#include <string>

namespace sp
{
    // Helper function to convert SPIRType to a string
    static std::string get_type_name(const spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type)
    {
        std::string type_str;

        switch (type.basetype)
        {
            case spirv_cross::SPIRType::Boolean: type_str = "bool"; break;
            case spirv_cross::SPIRType::Int: type_str = "int"; break;
            case spirv_cross::SPIRType::UInt: type_str = "uint"; break;
            case spirv_cross::SPIRType::Float: type_str = "float"; break;
            case spirv_cross::SPIRType::Double: type_str = "double"; break;
            case spirv_cross::SPIRType::Struct: return compiler.get_name(type.self);
            case spirv_cross::SPIRType::Image:
            case spirv_cross::SPIRType::SampledImage:
            {
                const auto& image_type = type;
                switch (image_type.image.dim)
                {
                    case spv::Dim1D: type_str = "sampler1D"; break;
                    case spv::Dim2D: type_str = "sampler2D"; break;
                    case spv::Dim3D: type_str = "sampler3D"; break;
                    case spv::DimCube: type_str = "samplerCube"; break;
                    default: type_str = "sampler"; break;
                }
                if (image_type.image.arrayed) type_str += "Array";
                return type_str;
            }
            default: type_str = "unknown"; break;
        }

        if (type.columns > 1)
        {
            return "mat" + std::to_string(type.columns);
        }
        if (type.vecsize > 1)
        {
            return "vec" + std::to_string(type.vecsize);
        }

        return type_str;
    }

    json ShaderReflector::generate_metadata(const std::vector<uint32_t>& spirv, const std::string& shader_type)
    {
        json metadata;
        metadata["shader_type"] = shader_type;

        const spirv_cross::Compiler        compiler{ spirv };
        const spirv_cross::ShaderResources resources{ compiler.get_shader_resources() };

        reflect_resources(compiler, resources.uniform_buffers, "uniform_buffers", metadata);
        reflect_resources(compiler, resources.storage_buffers, "storage_buffers", metadata);
        reflect_resources(compiler, resources.stage_inputs, "stage_inputs", metadata);
        reflect_resources(compiler, resources.stage_outputs, "stage_outputs", metadata);
        reflect_resources(compiler, resources.sampled_images, "sampled_images", metadata);
        reflect_resources(compiler, resources.storage_images, "storage_images", metadata);
        reflect_push_constants(compiler, resources, metadata, shader_type);

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

            const auto& type = compiler.get_type(resource.type_id);
            j_resource["type"] = get_type_name(compiler, type);

            if (type_name != "stage_inputs" && type_name != "stage_outputs")
            {
                j_resource["set"]     = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                j_resource["binding"] = compiler.get_decoration(resource.id, spv::DecorationBinding);
            }
            else j_resource["location"] = compiler.get_decoration(resource.id, spv::DecorationLocation);

            if (type_name == "uniform_buffers" || type_name == "storage_buffers")
            {
                j_resource["size"] = compiler.get_declared_struct_size(compiler.get_type(resource.base_type_id));
            }
            else if (type_name == "stage_inputs" || type_name == "stage_outputs")
            {
                size_t size = (type.width / 8) * type.vecsize;
                if (type.columns > 1) size *= type.columns;
                j_resource["size"] = size;
            }

            j_resources.push_back(j_resource);
        }

        metadata[type_name] = j_resources;
    }

    void ShaderReflector::reflect_push_constants(
        const spirv_cross::Compiler&        compiler,
        const spirv_cross::ShaderResources& resources,
        json&                               metadata,
        const std::string&                  shader_type)
    {
        if (resources.push_constant_buffers.empty()) return;
        json push_constants_array = json::array();

        for (const auto& push_constant_resource : resources.push_constant_buffers)
        {
            json        j_pc;
            const auto& type = compiler.get_type(push_constant_resource.base_type_id);

            j_pc["name"] = compiler.get_name(push_constant_resource.id);
            j_pc["type"] = compiler.get_name(type.self);
            j_pc["shader_stage"] = shader_type;

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
                const auto& member_type = compiler.get_type(type.member_types[i]);
                member["name"]   = compiler.get_member_name(type.self, i);
                member["type"]   = get_type_name(compiler, member_type);
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
