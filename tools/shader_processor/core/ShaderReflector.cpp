#include "ShaderReflector.hpp"
#include <string>

namespace sp
{
    static std::string get_image_format_name(const spv::ImageFormat format)
    {
        switch (format)
        {
            case spv::ImageFormatRgba32f: return "rgba32f";
            case spv::ImageFormatRgba16f: return "rgba16f";
            case spv::ImageFormatR32f: return "r32f";
            case spv::ImageFormatRgba8: return "rgba8";
            case spv::ImageFormatRgba8Snorm: return "rgba8_snorm";
            case spv::ImageFormatRg32f: return "rg32f";
            case spv::ImageFormatRg16f: return "rg16f";
            case spv::ImageFormatR11fG11fB10f: return "r11f_g11f_b10f";
            case spv::ImageFormatR16f: return "r16f";
            case spv::ImageFormatRgba16: return "rgba16";
            case spv::ImageFormatRgb10A2: return "rgb10_a2";
            case spv::ImageFormatRg16: return "rg16";
            case spv::ImageFormatRg8: return "rg8";
            case spv::ImageFormatR16: return "r16";
            case spv::ImageFormatR8: return "r8";
            case spv::ImageFormatRgba16Snorm: return "rgba16_snorm";
            case spv::ImageFormatRg16Snorm: return "rg16_snorm";
            case spv::ImageFormatRg8Snorm: return "rg8_snorm";
            case spv::ImageFormatR16Snorm: return "r16_snorm";
            case spv::ImageFormatR8Snorm: return "r8_snorm";
            case spv::ImageFormatRgba32i: return "rgba32i";
            case spv::ImageFormatRgba16i: return "rgba16i";
            case spv::ImageFormatRgba8i: return "rgba8i";
            case spv::ImageFormatR32i: return "r32i";
            case spv::ImageFormatRg32i: return "rg32i";
            case spv::ImageFormatRg16i: return "rg16i";
            case spv::ImageFormatRg8i: return "rg8i";
            case spv::ImageFormatR16i: return "r16i";
            case spv::ImageFormatR8i: return "r8i";
            case spv::ImageFormatRgba32ui: return "rgba32ui";
            case spv::ImageFormatRgba16ui: return "rgba16ui";
            case spv::ImageFormatRgba8ui: return "rgba8ui";
            case spv::ImageFormatR32ui: return "r32ui";
            case spv::ImageFormatRgb10a2ui: return "rgb10_a2ui";
            case spv::ImageFormatRg32ui: return "rg32ui";
            case spv::ImageFormatRg16ui: return "rg16ui";
            case spv::ImageFormatRg8ui: return "rg8ui";
            case spv::ImageFormatR16ui: return "r16ui";
            case spv::ImageFormatR8ui: return "r8ui";
            case spv::ImageFormatUnknown:
            default: return "unknown";
        }
    }

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
                const bool is_storage = type.basetype == spirv_cross::SPIRType::Image;
                const std::string prefix = is_storage ? "image" : "sampler";

                switch (image_type.image.dim)
                {
                    case spv::Dim1D: type_str = prefix + "1D"; break;
                    case spv::Dim2D: type_str = prefix + "2D"; break;
                    case spv::Dim3D: type_str = prefix + "3D"; break;
                    case spv::DimCube: type_str = prefix + "Cube"; break;
                    default: type_str = prefix; break;
                }

                if (image_type.image.arrayed) type_str += "Array";
                return type_str;
            }
            default: type_str = "unknown"; break;
        }

        if (type.columns > 1) return "mat" + std::to_string(type.columns);
        if (type.vecsize > 1)  return "vec" + std::to_string(type.vecsize);

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

        if (shader_type == "compute") reflect_compute_work_group_size(compiler, metadata);

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
                const auto& buffer_type = compiler.get_type(resource.base_type_id);
                j_resource["size"] = compiler.get_declared_struct_size(buffer_type);

                // Reflect struct members
                json members = json::array();
                for (uint32_t i = 0; i < buffer_type.member_types.size(); ++i)
                {
                    json member;
                    const auto& member_type = compiler.get_type(buffer_type.member_types[i]);
                    member["name"] = compiler.get_member_name(buffer_type.self, i);
                    member["type"] = get_type_name(compiler, member_type);
                    member["offset"] = compiler.get_member_decoration(buffer_type.self, i, spv::DecorationOffset);
                    member["size"] = compiler.get_declared_struct_member_size(buffer_type, i);

                    // Add array information if applicable
                    if (!member_type.array.empty()) member["array_size"] = member_type.array[0];

                    members.push_back(member);
                }
                j_resource["members"] = members;
            }
            else if (type_name == "stage_inputs" || type_name == "stage_outputs")
            {
                size_t size = type.width / 8 * type.vecsize;
                if (type.columns > 1) size *= type.columns;
                j_resource["size"] = size;
            }
            else if (type_name == "storage_images")
            {
                const auto& image_type = type;
                j_resource["format"] = get_image_format_name(image_type.image.format);

                const auto access = compiler.get_decoration(resource.id, spv::DecorationNonReadable);
                const auto writable = compiler.get_decoration(resource.id, spv::DecorationNonWritable);

                if (access && !writable) j_resource["access"] = "writeonly";
                else if (!access && writable) j_resource["access"] = "readonly";
                else j_resource["access"] = "readwrite";
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

            j_pc["offset"] = 0;
            j_pc["size"]   = compiler.get_declared_struct_size(type);

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

    void ShaderReflector::reflect_compute_work_group_size(
        const spirv_cross::Compiler& compiler,
        json&                        metadata)
    {
        const auto& entry_points = compiler.get_entry_points_and_stages();
        for (const auto& [name, execution_model] : entry_points)
        {
            if (execution_model == spv::ExecutionModelGLCompute)
            {
                spirv_cross::SpecializationConstant x_spec, y_spec, z_spec;
                compiler.get_work_group_size_specialization_constants(x_spec, y_spec, z_spec);

                uint32_t x = 1, y = 1, z = 1;

                if (x_spec.id != 0) x = compiler.get_constant(x_spec.id).scalar();
                if (y_spec.id != 0) y = compiler.get_constant(y_spec.id).scalar();
                if (z_spec.id != 0) z = compiler.get_constant(z_spec.id).scalar();

                if (x == 1 && y == 1 && z == 1)
                {
                    const auto sizes = compiler.get_entry_point(name, execution_model).workgroup_size;
                    x = sizes.x;
                    y = sizes.y;
                    z = sizes.z;
                }

                json work_group_size;
                work_group_size["x"] = x;
                work_group_size["y"] = y;
                work_group_size["z"] = z;

                metadata["work_group_size"] = work_group_size;
                break;
            }
        }
    }
}
