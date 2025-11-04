#include "MaterialSystem.hpp"
#include "boza/rendering/Texture.hpp"
#include "ShaderMetadata.hpp"
#include "boza/core/Logger.hpp"
#include "boza/AssetPaths.hpp"
#include "boza/rhi/Factory.hpp"
#include <fstream>

namespace boza
{
    namespace
    {
        MaterialPropertyValue parse_property_value(const json& value)
        {
            if (value.is_number_float()) return value.get<float>();
            if (value.is_number_integer()) return value.get<int>();

            if (value.is_array())
            {
                if (value.size() == 2) return glm::vec2(value[0].get<float>(), value[1].get<float>());
                if (value.size() == 3)
                    return glm::vec3(value[0].get<float>(), value[1].get<float>(),
                                     value[2].get<float>());
                if (value.size() == 4)
                    return glm::vec4(value[0].get<float>(), value[1].get<float>(),
                                     value[2].get<float>(), value[3].get<float>());
            }

            return {};
        }
    }


    MaterialSystem::MaterialSystem(const GraphicsApi api, rhi::Device* device, const uint32_t frames_in_flight)
        : api_(api),
          device_(device),
          frames_in_flight_(frames_in_flight) { create_default_texture(); }

    MaterialSystem::~MaterialSystem() = default;

    Material* MaterialSystem::material(const std::string& name)
    {
        if (materials_.contains(name)) return materials_.at(name).get();

        if (!material_definitions_.contains(name))
        {
            MaterialDefinition def = parse_material_definition(name);
            if (def.vertex_shader.empty() || def.fragment_shader.empty())
            {
                Logger::error("Failed to load material definition for '{}'", name);
                return nullptr;
            }
            material_definitions_[name] = std::move(def);
        }

        const auto& def = material_definitions_.at(name);

        auto material = std::make_unique<Material>(name, def, this);

        if (!material->initialize())
        {
            Logger::error("Failed to initialize material '{}'", name);
            return nullptr;
        }

        auto* material_ptr = material.get();
        materials_[name]   = std::move(material);
        return material_ptr;
    }

    Material* MaterialSystem::create_material_instance(
        const std::string& instance_name,
        const std::string& base_material_name,
        const json&        property_overrides)
    {
        if (materials_.contains(instance_name))
        {
            Logger::warn("Material instance '{}' already exists. Returning existing instance.", instance_name);
            return materials_.at(instance_name).get();
        }

        if (!material_definitions_.contains(base_material_name))
        {
            MaterialDefinition base_def = parse_material_definition(base_material_name);
            if (base_def.vertex_shader.empty() || base_def.fragment_shader.empty())
            {
                Logger::error("Failed to load base material '{}' for instance '{}'", base_material_name, instance_name);
                return nullptr;
            }
            material_definitions_[base_material_name] = std::move(base_def);
        }

        MaterialDefinition new_def = material_definitions_.at(base_material_name);

        if (property_overrides.contains("textures"))
        {
            for (auto& [key, val] : property_overrides["textures"].items()) new_def.textures[key] = val.get<
                std::string>();
        }

        if (property_overrides.contains("properties"))
        {
            for (auto& [key, val] : property_overrides["properties"].items()) new_def.properties[key] =
                    parse_property_value(val);
        }

        auto material = std::make_unique<Material>(instance_name, new_def, this);

        if (!material->initialize())
        {
            Logger::error("Failed to initialize material instance '{}'", instance_name);
            return nullptr;
        }

        auto* material_ptr        = material.get();
        materials_[instance_name] = std::move(material);
        return material_ptr;
    }

    void MaterialSystem::load_materials_for_strategy(const MaterialLoadStrategy strategy)
    {
        const auto material_files = AssetPaths::get_all_material_files();

        for (const auto& path : material_files)
        {
            std::string material_name = path.stem().stem().string();
            if (materials_.contains(material_name)) continue;

            const MaterialDefinition def = parse_material_definition(path.string());
            if (def.load_strategy == strategy) { material(material_name); }
        }
    }

    void MaterialSystem::destroy()
    {
        for (const auto& material : materials_ | std::views::values) if (material) material->destroy();
        for (const auto& sampler : sampler_cache_  | std::views::values) if (sampler) sampler->destroy();

        materials_.clear();
        texture_cache_.clear();
        sampler_cache_.clear();
        default_texture_.reset();
    }

    Texture* MaterialSystem::get_or_load_texture(const std::string& texture_path)
    {
        if (texture_cache_.contains(texture_path)) { return texture_cache_.at(texture_path).get(); }

        std::string full_path = AssetPaths::resolve_texture(texture_path).string();

        auto texture = std::make_unique<Texture>(
            api_, device_, Texture::Descriptor{
                .width = 1,
                .height = 1,
                .format = Texture::Format::RGBA8,
                .usage = Texture::Usage::Sampled
            });

        if (!texture || !texture->load_from_file(full_path))
        {
            Logger::error("Failed to load texture '{}'", full_path);
            return nullptr;
        }

        Texture* texture_ptr         = texture.get();
        texture_cache_[texture_path] = std::move(texture);
        return texture_ptr;
    }

    rhi::Sampler* MaterialSystem::get_or_create_sampler(rhi::SamplerFilter filter)
    {
        const uint32_t key = static_cast<uint32_t>(filter);
        if (sampler_cache_.contains(key)) return sampler_cache_.at(key).get();

        std::unique_ptr<rhi::Sampler> sampler(rhi::create_sampler(api_, { .device = device_, .filter = filter }));
        if (!sampler)
        {
            Logger::error("Failed to create sampler");
            return nullptr;
        }

        rhi::Sampler* sampler_ptr = sampler.get();
        sampler_cache_[key]       = std::move(sampler);
        return sampler_ptr;
    }

    void MaterialSystem::upload_dirty_materials(const uint32_t frame_index)
    {
        for (const auto& material : materials_ | std::views::values)
            if (material->is_dirty()) material->upload_to_gpu(frame_index);
    }

    MaterialDefinition MaterialSystem::parse_material_definition(const std::string& filepath)
    {
        std::filesystem::path full_path;
        if (std::filesystem::exists(filepath)) full_path = filepath;
        else full_path = AssetPaths::resolve_material(filepath);

        std::ifstream file(full_path);
        if (!file.is_open())
        {
            Logger::error("Failed to open material file: {}", full_path.string());
            return {};
        }

        json data;
        try { file >> data; }
        catch (json::parse_error& e)
        {
            Logger::error("Failed to parse material file '{}': {}", full_path.string(), e.what());
            return {};
        }

        MaterialDefinition def;

        if (data.contains("base"))
        {
            const std::string base_path = data["base"];
            def = parse_material_definition(base_path);
        }

        if (data.contains("vertex_shader")) def.vertex_shader = data["vertex_shader"];
        if (data.contains("fragment_shader")) def.fragment_shader = data["fragment_shader"];

        if (data.contains("textures"))
        {
            for (auto& [key, val] : data["textures"].items())
                def.textures[key] = val.get<std::string>();
        }

        const json* props = nullptr;
        if (data.contains("properties")) { props = &data["properties"]; }
        else if (data.contains("material_properties")) { props = &data["material_properties"]; }

        if (props)
        {
            for (auto& [key, val] : props->items())
                def.properties[key] = parse_property_value(val);
        }

        return def;
    }

    void MaterialSystem::create_default_texture()
    {
        constexpr uint32_t white_pixel = 0xFFFFFFFF;

        default_texture_ = std::make_unique<Texture>(
            api_, device_, Texture::Descriptor{
                .width = 1,
                .height = 1,
                .format = Texture::Format::RGBA8,
                .usage = Texture::Usage::Sampled
            });

        if (default_texture_) default_texture_->upload(&white_pixel, sizeof(uint32_t));
    }
}
