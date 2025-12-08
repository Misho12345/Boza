module boza.app:material_loader;

import std;
import boza.common;
import boza.gfx;
import boza.rhi;

namespace boza::app
{
    enum class LoadStrategy : std::uint8_t
    {
        GameLoad,
        OnDemand
    };

    struct TextureInfo
    {
        std::string   file;
        TextureFormat format{ TextureFormat::RGBA8 };
    };

    struct MaterialDefinition
    {
        std::string   name;
        std::string   vertex_shader;
        std::string   fragment_shader;
        LoadStrategy  load_strategy{ LoadStrategy::GameLoad };

        std::unordered_map<std::string, TextureInfo> textures;
        std::unordered_map<std::string, glm::vec4> vec4_properties;
    };

    class MaterialLoader final
    {
    public:
        MaterialLoader() = default;
        ~MaterialLoader();

        MaterialLoader(const MaterialLoader&) = delete;
        MaterialLoader& operator=(const MaterialLoader&) = delete;
        MaterialLoader(MaterialLoader&&) = delete;
        MaterialLoader& operator=(MaterialLoader&&) = delete;

        bool initialize(
            rhi::Device*         device,
            rhi::Swapchain*      swapchain,
            rhi::DescriptorPool* descriptor_pool,
            rhi::ResourceCache*  resource_cache,
            rhi::GraphicsApi     api);

        void shutdown();

        bool load_all_material_definitions();
        bool create_game_load_materials();

        Material* get_or_create_material(const std::string& name);
        Material* get_material(const std::string& name) const;

        void register_material(const std::string& name, Material* material);
        void bind_engine_resources(Material* material);

        [[nodiscard]] Texture* error_texture() const { return error_texture_; }
        [[nodiscard]] rhi::Sampler* default_sampler() const { return default_sampler_.get(); }

        [[nodiscard]] rhi::Buffer* camera_ubo() const { return camera_ubo_.get(); }
        [[nodiscard]] rhi::Buffer* light_ubo() const { return light_ubo_.get(); }
        [[nodiscard]] rhi::Buffer* time_ubo() const { return time_ubo_.get(); }

        void update_time_ubo(float time, float delta_time);

    private:
        std::optional<MaterialDefinition> load_material_definition(const fs::path& path);
        Material* create_material_from_definition(const MaterialDefinition& def);
        void setup_material_from_definition(Material* material, const MaterialDefinition& def);

        rhi::Device*         device_{ nullptr };
        rhi::Swapchain*      swapchain_{ nullptr };
        rhi::DescriptorPool* descriptor_pool_{ nullptr };
        rhi::ResourceCache*  resource_cache_{ nullptr };
        rhi::GraphicsApi     api_{};

        std::unordered_map<std::string, MaterialDefinition> definitions_;
        std::unordered_map<std::string, Material*> materials_;
        std::unordered_map<std::string, Texture*> loaded_textures_;

        Texture* error_texture_{ nullptr };
        std::unique_ptr<rhi::Sampler> default_sampler_;

        std::unique_ptr<rhi::Buffer> camera_ubo_;
        std::unique_ptr<rhi::Buffer> light_ubo_;
        std::unique_ptr<rhi::Buffer> time_ubo_;

        bool initialized_{ false };
    };
}

