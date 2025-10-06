#include "boza/rhi/ShaderModule.hpp"
#include <nlohmann/json.hpp>

#include "boza/core/Logger.hpp"
#include "boza/AssetPaths.hpp"

namespace boza::rhi
{
    using json = nlohmann::json;

    namespace
    {
        ShaderStage shader_stage_from_string(const std::string& s)
        {
            if (s == "vertex") return ShaderStage::Vertex;
            if (s == "fragment") return ShaderStage::Fragment;
            if (s == "compute") return ShaderStage::Compute;
            if (s == "geometry") return ShaderStage::Geometry;
            if (s == "tess_control") return ShaderStage::TessControl;
            if (s == "tess_evaluation") return ShaderStage::TessEvaluation;
            return ShaderStage::None;
        }

        ShaderDataType parse_data_type(const std::string& type_str)
        {
            if (type_str == "bool") return ShaderDataType::Bool;
            if (type_str == "int") return ShaderDataType::Int;
            if (type_str == "uint") return ShaderDataType::Uint;
            if (type_str == "float") return ShaderDataType::Float;
            if (type_str == "double") return ShaderDataType::Double;
            if (type_str == "vec2") return ShaderDataType::Vec2;
            if (type_str == "vec3") return ShaderDataType::Vec3;
            if (type_str == "vec4") return ShaderDataType::Vec4;
            if (type_str == "ivec2") return ShaderDataType::IVec2;
            if (type_str == "ivec3") return ShaderDataType::IVec3;
            if (type_str == "ivec4") return ShaderDataType::IVec4;
            if (type_str == "uvec2") return ShaderDataType::UVec2;
            if (type_str == "uvec3") return ShaderDataType::UVec3;
            if (type_str == "uvec4") return ShaderDataType::UVec4;
            if (type_str == "mat2") return ShaderDataType::Mat2;
            if (type_str == "mat3") return ShaderDataType::Mat3;
            if (type_str == "mat4") return ShaderDataType::Mat4;
            if (type_str == "sampler1D") return ShaderDataType::Sampler1D;
            if (type_str == "sampler2D") return ShaderDataType::Sampler2D;
            if (type_str == "sampler3D") return ShaderDataType::Sampler3D;
            if (type_str == "samplerCube") return ShaderDataType::SamplerCube;
            if (type_str == "sampler1DArray") return ShaderDataType::Sampler1DArray;
            if (type_str == "sampler2DArray") return ShaderDataType::Sampler2DArray;
            if (type_str == "samplerCubeArray") return ShaderDataType::SamplerCubeArray;
            return ShaderDataType::Unknown;
        }

        void parse_resources(const json& j, const std::string& type, std::unordered_map<std::string, ShaderModule::ShaderResource>& resources)
        {
            if (!j.contains(type)) return;

            for (const auto& res : j[type])
            {
                ShaderModule::ShaderResource resource;
                if (res.contains("set")) res.at("set").get_to(resource.set);
                if (res.contains("binding")) res.at("binding").get_to(resource.binding);
                if (res.contains("location")) res.at("location").get_to(resource.location);
                if (res.contains("size")) res.at("size").get_to(resource.size);
                if (res.contains("vec_size")) res.at("vec_size").get_to(resource.vec_size);
                if (res.contains("columns")) res.at("columns").get_to(resource.columns);
                if (res.contains("type"))
                {
                    res.at("type").get_to(resource.type_name);
                    resource.data_type = parse_data_type(resource.type_name);
                }
                resources[res["name"]] = resource;
            }
        }

        void parse_push_constants(const json& j, std::unordered_map<std::string, ShaderModule::PushConstant>& push_constants)
        {
            if (!j.contains("push_constants")) return;

            for (const auto& pc_json : j["push_constants"])
            {
                std::string stage_str;
                if(pc_json.contains("shader_stage"))
                {
                    pc_json.at("shader_stage").get_to(stage_str);
                }

                ShaderModule::PushConstant range
                {
                    .stage = shader_stage_from_string(stage_str),
                    .offset = static_cast<uint32_t>(pc_json.value("offset", 0)),
                    .size = static_cast<uint32_t>(pc_json.value("size", 0))
                };

                // Parse members
                if (pc_json.contains("members"))
                {
                    for (const auto& member_json : pc_json["members"])
                    {
                        ShaderModule::PushConstantMember member;
                        if (member_json.contains("name")) member_json.at("name").get_to(member.name);
                        if (member_json.contains("type")) member_json.at("type").get_to(member.type_name);
                        if (member_json.contains("offset")) member_json.at("offset").get_to(member.offset);
                        if (member_json.contains("size")) member_json.at("size").get_to(member.size);
                        member.data_type = parse_data_type(member.type_name);
                        range.members.push_back(member);
                    }
                }

                push_constants[pc_json["name"]] = range;
            }
        }
    }

    ShaderDataType ShaderModule::parse_shader_data_type(const std::string& type_str)
    {
        return parse_data_type(type_str);
    }

    bool ShaderModule::get_meta_data()
    {
        const fs::path shader_dir = AssetPaths::get_shaders_dir();
        const fs::path path = shader_dir / desc.filename;
        const fs::path meta_path = path / (fs::path(desc.filename).stem().string() + ".meta.json");
        const std::vector<uint8_t> meta_file_data = read_file<uint8_t>(meta_path);

        if (meta_file_data.empty()) return false;

        const json meta_json = json::parse(meta_file_data, nullptr, false);

        if (meta_json.is_discarded())
        {
            Logger::error("Failed to parse shader metadata json for {}", desc.filename);
            return false;
        }

        parse_resources(meta_json, "uniform_buffers", meta_data_.uniform_buffers);
        parse_resources(meta_json, "storage_buffers", meta_data_.storage_buffers);
        parse_resources(meta_json, "stage_inputs", meta_data_.stage_inputs);
        parse_resources(meta_json, "stage_outputs", meta_data_.stage_outputs);
        parse_resources(meta_json, "subpass_inputs", meta_data_.subpass_inputs);
        parse_resources(meta_json, "sampled_images", meta_data_.sampled_images);
        parse_resources(meta_json, "storage_images", meta_data_.storage_images);
        parse_push_constants(meta_json, meta_data_.push_constants);

        return true;
    }
}
