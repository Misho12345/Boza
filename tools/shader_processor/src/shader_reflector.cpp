module shader_processor;

import :shader_reflector;

import <nlohmann/json.hpp>;
using nlohmann::json;

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

        if (type.columns > 1)
        {
            const std::string prefix = type.basetype == spirv_cross::SPIRType::Double ? "dmat" : "mat";
            if (type.vecsize == type.columns) return prefix + std::to_string(type.columns);
            return prefix + std::to_string(type.columns) + "x" + std::to_string(type.vecsize);
        }
        if (type.vecsize > 1)
        {
            switch (type.basetype)
            {
                case spirv_cross::SPIRType::Boolean: type_str = "bvec"; break;
                case spirv_cross::SPIRType::Int: type_str = "ivec"; break;
                case spirv_cross::SPIRType::UInt: type_str = "uvec"; break;
                case spirv_cross::SPIRType::Double: type_str = "dvec"; break;
                default: type_str = "vec"; break;
            }
            return type_str + std::to_string(type.vecsize);
        }

        return type_str;
    }

    static std::string get_buffer_access(const spirv_cross::Compiler& compiler, uint32_t resource_id)
    {
        const bool is_readonly = compiler.get_decoration(resource_id, spv::DecorationNonWritable);
        const bool is_writeonly = compiler.get_decoration(resource_id, spv::DecorationNonReadable);
        const bool is_coherent = compiler.has_decoration(resource_id, spv::DecorationCoherent);

        std::string access;
        if (is_readonly && !is_writeonly) access = "readonly";
        else if (is_writeonly && !is_readonly) access = "writeonly";
        else access = "readwrite";

        if (is_coherent) access += "_coherent";

        return access;
    }

    json ShaderReflector::generate_metadata(const std::vector<std::uint32_t>& spirv, const std::string& shader_type)
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

            const auto& buffer_type = compiler.get_type(resource.base_type_id);
            std::string resource_name = compiler.get_name(resource.id);
            if (resource_name.empty())
            {
                resource_name = compiler.get_name(buffer_type.self);
            }
            j_resource["name"] = resource_name;

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
                size_t buffer_size = compiler.get_declared_struct_size(buffer_type);

                // Check if buffer contains runtime-sized arrays
                bool has_runtime_array = false;
                for (uint32_t i = 0; i < buffer_type.member_types.size(); ++i)
                {
                    const auto& member_type = compiler.get_type(buffer_type.member_types[i]);
                    if (!member_type.array.empty() && member_type.array[0] == 0)
                    {
                        has_runtime_array = true;
                        break;
                    }
                }

                j_resource["min_size"] = buffer_size;
                j_resource["is_runtime_sized"] = has_runtime_array;

                // Add access qualifier for storage buffers
                if (type_name == "storage_buffers")
                {
                    j_resource["access"] = get_buffer_access(compiler, resource.id);
                }

                json members = json::array();
                for (uint32_t i = 0; i < buffer_type.member_types.size(); ++i)
                {
                    json member;
                    const auto& member_type = compiler.get_type(buffer_type.member_types[i]);

                    member["name"] = compiler.get_member_name(buffer_type.self, i);
                    member["type"] = get_type_name(compiler, member_type);
                    member["offset"] = compiler.get_member_decoration(buffer_type.self, i, spv::DecorationOffset);

                    size_t member_size = compiler.get_declared_struct_member_size(buffer_type, i);

                    // Handle arrays
                    if (!member_type.array.empty())
                    {
                        uint32_t array_size = member_type.array[0];

                        if (array_size == 0)
                        {
                            // Runtime-sized array
                            member["is_runtime_array"] = true;
                            member["array_size"] = 0;
                            member["min_size"] = 0;

                            // Calculate stride - this is the key info needed!
                            uint32_t stride;
                            if (member_type.basetype == spirv_cross::SPIRType::Struct)
                            {
                                // For struct arrays, get the struct size
                                const auto& element_type = compiler.get_type(member_type.self);
                                stride = static_cast<uint32_t>(compiler.get_declared_struct_size(element_type));
                            }
                            else
                            {
                                // For primitive type arrays, calculate stride
                                stride = member_type.width / 8 * member_type.vecsize;
                                if (member_type.columns > 1) stride *= member_type.columns;
                            }
                            member["array_stride"] = stride;
                        }
                        else
                        {
                            // Fixed-size array
                            member["is_runtime_array"] = false;
                            member["array_size"] = array_size;
                            member["min_size"] = member_size;
                        }
                    }
                    else member["min_size"] = member_size;

                    members.push_back(member);
                }
                j_resource["members"] = members;
            }
            else if (type_name == "stage_inputs" || type_name == "stage_outputs")
            {
                size_t size = type.width / 8 * type.vecsize;
                if (type.columns > 1) size *= type.columns;
                j_resource["min_size"] = size;
            }
            else if (type_name == "sampled_images")
            {
                const auto& image_type = type;
                j_resource["format"] = get_image_format_name(image_type.image.format);
                j_resource["vec_size"] = image_type.vecsize;
                j_resource["columns"] = image_type.columns;
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
            j_pc["size"] = compiler.get_declared_struct_size(type);

            json members = json::array();
            for (uint32_t i = 0; i < type.member_types.size(); ++i)
            {
                json member;
                const auto& member_type = compiler.get_type(type.member_types[i]);
                member["name"]   = compiler.get_member_name(type.self, i);
                member["type"]   = get_type_name(compiler, member_type);
                member["offset"] = compiler.get_member_decoration(type.self, i, spv::DecorationOffset);
                member["size"]   = compiler.get_declared_struct_member_size(type, i);
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