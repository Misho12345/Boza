#include "boza/rhi/ShaderModule.hpp"
#include <nlohmann/json.hpp>

#include "boza/core/Logger.hpp"

namespace boza::rhi
{
    using json = nlohmann::json;

    namespace
    {
        void parse_resources(const json& j, const std::string& type, std::unordered_map<std::string, ShaderModule::ResourceBinding>& resources)
        {
            if (!j.contains(type)) return;

            for (const auto& res : j[type])
            {
                ShaderModule::ResourceBinding binding;
                if (res.contains("set")) res.at("set").get_to(binding.set);
                if (res.contains("binding")) res.at("binding").get_to(binding.binding);
                if (res.contains("location")) res.at("location").get_to(binding.location);
                resources[res["name"]] = binding;
            }
        }

        void parse_push_constants(const json& j, std::unordered_map<std::string, ShaderModule::PushConstantRange>& push_constants)
        {
            if (!j.contains("push_constants")) return;

            for (const auto& pc_json : j["push_constants"])
            {
                const ShaderModule::PushConstantRange range
                {
                    .offset = static_cast<uint32_t>(pc_json.value("offset", 0)),
                    .size = static_cast<uint32_t>(pc_json.value("size", 0))
                };

                push_constants[pc_json["name"]] = range;
            }
        }
    }

    bool ShaderModule::get_meta_data()
    {
        const fs::path path = fs::current_path() / "shaders" / desc.filename;
        const fs::path meta_path = path / (fs::path(desc.filename).stem().string() + ".meta.json");
        const std::vector<uint8_t> meta_data = read_file<uint8_t>(meta_path);

        if (meta_data.empty()) return false;

        const json meta_json = json::parse(meta_data, nullptr, false);

        if (meta_json.is_discarded())
        {
            Logger::error("Failed to parse shader metadata json for {}", desc.filename);
            return false;
        }

        parse_resources(meta_json, "uniform_buffers", meta_data_.uniform_buffers);
        parse_resources(meta_json, "storage_buffers", meta_data_.storage_buffers);
        parse_resources(meta_json, "stage_inputs", meta_data_.stage_inputs);
        parse_resources(meta_json, "stage_outputs", meta_data_.stage_outputs);
        parse_resources(meta_json, "sampled_images", meta_data_.sampled_images);
        parse_resources(meta_json, "storage_images", meta_data_.storage_images);
        parse_push_constants(meta_json, meta_data_.push_constants);

        return true;
    }
}
