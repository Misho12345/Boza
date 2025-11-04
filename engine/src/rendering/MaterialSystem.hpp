#pragma once
#include "boza/pch.hpp"
#include "boza/core/IMaterialProvider.hpp"
#include "boza/GraphicsApi.hpp"
#include "boza/rendering/Material.hpp"
#include "boza/rendering/MaterialCommon.hpp"

namespace boza
{
    namespace rhi
    {
        class Device;
        class Buffer;
        class Sampler;
        enum class SamplerFilter : uint8_t;
    }

    class Texture;
    class ShaderMetadata;

    class MaterialSystem final : public IMaterialProvider
    {
    public:
        explicit MaterialSystem(GraphicsApi api, rhi::Device* device, uint32_t frames_in_flight);
        ~MaterialSystem() override;

        Material* material(const std::string& name) override;

        Material* create_material_instance(
            const std::string& instance_name,
            const std::string& base_material_name,
            const json&        property_overrides = json::object()
        );

        void          load_materials_for_strategy(MaterialLoadStrategy strategy);
        void          destroy();
        Texture*      get_or_load_texture(const std::string& texture_path);
        rhi::Sampler* get_or_create_sampler(rhi::SamplerFilter filter);
        Texture*      get_default_texture() const { return default_texture_.get(); }
        void          upload_dirty_materials(uint32_t frame_index = 0);

        GraphicsApi  api() const { return api_; }
        rhi::Device* device() const { return device_; }
        uint32_t     frames_in_flight() const { return frames_in_flight_; }

    private:
        static MaterialDefinition parse_material_definition(const std::string& filepath);

        void create_default_texture();

        GraphicsApi  api_;
        rhi::Device* device_;
        uint32_t     frames_in_flight_;

        std::unordered_map<std::string, std::unique_ptr<Material>>  materials_;
        std::unordered_map<std::string, MaterialDefinition>         material_definitions_;
        std::unordered_map<std::string, std::unique_ptr<Texture>>   texture_cache_;
        std::unordered_map<uint32_t, std::unique_ptr<rhi::Sampler>> sampler_cache_;

        std::unique_ptr<Texture> default_texture_;
    };
}
