#include "ShaderMetadata.hpp"
#include "boza/AssetPaths.hpp"
#include "boza/core/Logger.hpp"
#include <fstream>

namespace boza
{
    ShaderMetadata::ShaderMetadata([[maybe_unused]] const std::string& vertex_shader, const std::string& fragment_shader)
    {
        auto metadata_path = (AssetPaths::resolve_shader(fragment_shader + ".frag") / (fragment_shader + ".meta.json")).string();

        if (!load_from_file(metadata_path))
        {
            Logger::warn("Could not load shader metadata from '{}'. Material properties may not work correctly.", metadata_path);
        }
    }

    bool ShaderMetadata::load_from_file(const std::string& metadata_path)
    {
        std::ifstream file(metadata_path);

        if (!file.is_open()) return false;

        nlohmann::json data;

        try
        {
            file >> data;
        }
        catch (nlohmann::json::parse_error& e)
        {
            Logger::error("Failed to parse shader metadata file '{}': {}", metadata_path, e.what());
            return false;
        }

        if (data.contains("shader_type")) shader_type_ = data["shader_type"];

        if (data.contains("uniform_buffers"))
        {
            for (const auto& ubo_json : data["uniform_buffers"])
            {
                UniformBufferInfo ubo_info;
                ubo_info.name = ubo_json["name"];
                ubo_info.set = ubo_json["set"];
                ubo_info.binding = ubo_json["binding"];
                ubo_info.size = ubo_json["size"];

                if (ubo_json.contains("members"))
                {
                    for (const auto& member_json : ubo_json["members"])
                    {
                        UniformBufferMember member;
                        member.name = member_json["name"];
                        member.type = member_json["type"];
                        member.offset = member_json["offset"];
                        member.size = member_json["size"];
                        member.array_size = member_json.value("array_size", 0);
                        ubo_info.members.push_back(member);
                    }
                }

                uniform_buffers_.push_back(ubo_info);
            }
        }

        loaded_ = true;
        return true;
    }

    const UniformBufferInfo* ShaderMetadata::get_uniform_buffer(const std::string& name) const
    {
        for (const auto& ubo : uniform_buffers_)
            if (ubo.name == name) return &ubo;

        return nullptr;
    }
}
