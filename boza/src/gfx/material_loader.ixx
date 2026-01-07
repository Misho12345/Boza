export module boza.gfx.material_loader;

import std;
import boza.common;
import boza.core;
import boza.gfx;
import boza.rhi;
import boza.detail;

export namespace boza::gfx
{
    enum class LoadStrategy : std::uint8_t
    {
        GameLoad,
        OnDemand
    };

    struct TextureInfo
    {
        std::string texture_file;
        std::string sampler_file;
    };

    using PropertyValue = std::variant<
        bool,
        std::int32_t,
        std::uint32_t,
        float,
        double,
        std::vector<float>,
        std::vector<std::int32_t>,
        std::vector<std::uint32_t>,
        std::vector<double>
    >;

    struct MaterialDefinition
    {
        std::string   name;
        std::string   vertex_shader;
        std::string   fragment_shader;
        LoadStrategy  load_strategy{ LoadStrategy::GameLoad };
        MaterialSettings settings{};

        flat_map<std::string, TextureInfo> textures;
        flat_map<std::string, flat_map<std::string, PropertyValue>> ubos;
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
            rhi::GraphicsApi     api);

        void shutdown();

        bool load_all_material_definitions();
        bool create_game_load_materials();

        Material& get_or_create_material(std::string_view name, const MaterialSettings& settings);
        Material* try_get_material(std::string_view name);

        void bind_engine_resources(const Material* material) const;

        [[nodiscard]] rhi::Buffer* camera_ubo() const { return camera_ubo_.get(); }
        [[nodiscard]] rhi::Buffer* light_ubo() const { return light_ubo_.get(); }
        [[nodiscard]] rhi::Buffer* time_ubo() const { return time_ubo_.get(); }

        void update_time_ubo(float time, float delta_time) const;

    private:
        MaterialLoader() = default;

        static std::optional<MaterialDefinition> load_material_definition(const fs::path& path);

        void destroy(std::string_view name);

        static Material create(
            std::string_view name,
            const MaterialSettings& settings);

        void setup(Material* material, const MaterialDefinition& def);

        rhi::Device*         device_{ nullptr };
        rhi::Swapchain*      swapchain_{ nullptr };
        rhi::DescriptorPool* descriptor_pool_{ nullptr };
        rhi::ResourceCache*  resource_cache_{ nullptr };
        rhi::GraphicsApi     api_{};

        flat_map<std::string, MaterialDefinition> definitions_;
        node_map<std::string, Material> materials_;

        std::unique_ptr<rhi::Buffer> camera_ubo_;
        std::unique_ptr<rhi::Buffer> light_ubo_;
        std::unique_ptr<rhi::Buffer> time_ubo_;

        bool initialized_{ false };

        friend class Material;
    };
}

