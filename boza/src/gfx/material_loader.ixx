export module boza.gfx.material_loader;

import std;
import boza.common;
import boza.core;
import boza.gfx;
import boza.rhi;
import boza.detail;
import boza.gfx.texture_loader;

export namespace boza::gfx
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

        flat_map<std::string, TextureInfo> textures;
        flat_map<std::string, glm::vec4> vec4_properties;
    };

    struct CameraUBO
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct LightUBO
    {
        glm::vec4 light_position;
        glm::vec4 light_color;
        glm::vec4 view_pos;
    };

    struct TimeUBO
    {
        float time;
        float delta_time;
        float padding[2];
    };

    class MaterialLoader final
    {
    public:
        ~MaterialLoader();
        static MaterialLoader& instance();

        MaterialLoader(const MaterialLoader&) = delete;
        MaterialLoader& operator=(const MaterialLoader&) = delete;
        MaterialLoader(MaterialLoader&&) = delete;
        MaterialLoader& operator=(MaterialLoader&&) = delete;

        bool initialize(
            rhi::Device*         device,
            rhi::Swapchain*      swapchain,
            rhi::DescriptorPool* descriptor_pool,
            rhi::ResourceCache*  resource_cache,
            TextureLoader*       texture_loader,
            rhi::GraphicsApi     api);

        void shutdown();

        bool load_all_material_definitions();
        bool create_game_load_materials();

        Material* get_or_create_material(const std::string& name);
        Material* get_material(const std::string& name) const;

        void register_material(const std::string& name, Material* material);
        void bind_engine_resources(const Material* material) const;

        [[nodiscard]] rhi::Sampler* default_sampler() const { return default_sampler_.get(); }

        [[nodiscard]] rhi::Buffer* camera_ubo() const { return camera_ubo_.get(); }
        [[nodiscard]] rhi::Buffer* light_ubo() const { return light_ubo_.get(); }
        [[nodiscard]] rhi::Buffer* time_ubo() const { return time_ubo_.get(); }

        void update_time_ubo(float time, float delta_time) const;

    private:
        MaterialLoader() = default;

        static std::optional<MaterialDefinition> load_material_definition(const std::filesystem::path& path);
        static Material*                         create_material_from_definition(const MaterialDefinition& def);
        void                                     setup_material_from_definition(Material* material, const MaterialDefinition& def);

        rhi::Device*         device_{ nullptr };
        rhi::Swapchain*      swapchain_{ nullptr };
        rhi::DescriptorPool* descriptor_pool_{ nullptr };
        rhi::ResourceCache*  resource_cache_{ nullptr };
        TextureLoader*       texture_loader_{ nullptr };
        rhi::GraphicsApi     api_{};

        flat_map<std::string, MaterialDefinition> definitions_;
        flat_map<std::string, Material*> materials_;

        std::unique_ptr<rhi::Sampler> default_sampler_;

        std::unique_ptr<rhi::Buffer> camera_ubo_;
        std::unique_ptr<rhi::Buffer> light_ubo_;
        std::unique_ptr<rhi::Buffer> time_ubo_;

        bool initialized_{ false };
    };
}

