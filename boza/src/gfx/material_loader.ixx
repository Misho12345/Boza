export module boza.gfx:material_loader;

import std;
import boza.common;
import boza.core;
import boza.rhi;
import boza.detail;
import :material;
import :texture;
import :sampler;
import :light;
import :resource_registry;

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
        std::string      name;
        std::string      vertex_shader;
        std::string      fragment_shader;
        LoadStrategy     load_strategy{ LoadStrategy::GameLoad };
        MaterialSettings settings{};

        flat_map<std::string, TextureInfo> textures;
        flat_map<std::string, flat_map<std::string, PropertyValue>> ubos;
    };

    struct CameraUBO
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct LightingUBO
    {
        glm::vec4 view_pos;
        glm::vec4 ambient_color;
        glm::uvec4 light_counts;
        glm::uvec4 cluster_grid;
        glm::vec4 screen_params;
        glm::vec4 shadow_params;
        glm::vec4 directional_shadow_splits;
        std::array<glm::mat4, 4> directional_shadow_view_projections{ glm::identity<glm::mat4>() };
    };

    struct GpuPointLight
    {
        glm::vec4 position_range;
        glm::vec4 color_intensity;
        glm::vec4 shadow_data;
    };

    struct GpuDirectionalLight
    {
        glm::vec4 direction_shadow;
        glm::vec4 color_intensity;
    };

    struct GpuSpotLight
    {
        glm::vec4 position_range;
        glm::vec4 direction_angles;
        glm::vec4 color_intensity;
        glm::vec4 shadow_data;
    };

    struct GpuClusterRecord
    {
        glm::uvec4 point_spot_ranges;
    };

    struct GpuShadowCaster
    {
        glm::vec4 center_half_x;
        glm::vec4 axis_x_half_y;
        glm::vec4 axis_y_half_z;
        glm::vec4 axis_z_padding;
    };

    struct GpuClusterLightCounter
    {
        std::uint32_t next_index{ 0 };
        std::uint32_t overflow_count{ 0 };
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
        [[nodiscard]] bool exists(const Material* ptr) const;

        void bind_engine_resources(Material* material) const;
        bool ensure_frame_render_targets(std::uint32_t width, std::uint32_t height);

        [[nodiscard]] Buffer* camera_ubo() { return camera_ubo_ ? &camera_ubo_.value() : nullptr; }
        [[nodiscard]] Buffer* lighting_ubo() { return lighting_ubo_ ? &lighting_ubo_.value() : nullptr; }
        [[nodiscard]] Buffer* time_ubo() { return time_ubo_ ? &time_ubo_.value() : nullptr; }
        [[nodiscard]] Buffer* point_lights_buffer() { return point_lights_buffer_ ? &point_lights_buffer_.value() : nullptr; }
        [[nodiscard]] Buffer* directional_lights_buffer() { return directional_lights_buffer_ ? &directional_lights_buffer_.value() : nullptr; }
        [[nodiscard]] Buffer* spot_lights_buffer() { return spot_lights_buffer_ ? &spot_lights_buffer_.value() : nullptr; }
        [[nodiscard]] Buffer* cluster_records_buffer() { return cluster_records_buffer_ ? &cluster_records_buffer_.value() : nullptr; }
        [[nodiscard]] Buffer* cluster_light_indices_buffer() { return cluster_light_indices_buffer_ ? &cluster_light_indices_buffer_.value() : nullptr; }
        [[nodiscard]] Buffer* cluster_light_counter_buffer() { return cluster_light_counter_buffer_ ? &cluster_light_counter_buffer_.value() : nullptr; }
        [[nodiscard]] std::uint32_t cluster_record_count() const { return lighting_data_.cluster_grid.w; }
        [[nodiscard]] Texture* frame_depth_texture() const { return frame_depth_texture_; }
        [[nodiscard]] Texture* ssao_texture() const { return ssao_texture_; }
        [[nodiscard]] Sampler* renderer_sampler() const { return renderer_sampler_; }
        [[nodiscard]] std::uint64_t frame_render_target_revision() const { return frame_render_target_revision_; }

        void log_cluster_cull_feedback();
        void update_time_ubo(float time, float delta_time);

        void begin_light_update(
            const glm::vec3& view_position,
            const glm::mat4& view,
            const glm::mat4& projection,
            std::uint32_t screen_width,
            std::uint32_t screen_height,
            float near_clip,
            float far_clip);

        void add_point_light(const glm::vec3& position, const PointLight& light);
        void add_directional_light(const glm::vec3& direction, const DirectionalLight& light);
        void add_spot_light(const glm::vec3& position, const glm::vec3& direction, const SpotLight& light);
        void add_shadow_caster(
            const glm::vec3& center,
            const glm::vec3& axis_x,
            const glm::vec3& axis_y,
            const glm::vec3& axis_z,
            const glm::vec3& half_extents);
        void upload_light_buffers();

        [[nodiscard]] Texture* directional_shadow_map() const { return directional_shadow_map_; }
        [[nodiscard]] Sampler* directional_shadow_sampler() const { return directional_shadow_sampler_; }
        [[nodiscard]] std::uint32_t directional_shadow_map_size() const { return directional_shadow_map_size_; }
        [[nodiscard]] bool directional_shadow_enabled() const { return directional_shadow_enabled_; }
        [[nodiscard]] const std::array<glm::mat4, 4>& directional_shadow_view_projections() const
        {
            return lighting_data_.directional_shadow_view_projections;
        }
        [[nodiscard]] const glm::vec4& directional_shadow_splits() const
        {
            return lighting_data_.directional_shadow_splits;
        }
        [[nodiscard]] Texture* point_shadow_map() const { return point_shadow_map_; }
        [[nodiscard]] Sampler* point_shadow_sampler() const { return point_shadow_sampler_; }
        [[nodiscard]] Buffer* point_shadow_matrices_buffer()
        {
            return point_shadow_matrices_buffer_ ? &point_shadow_matrices_buffer_.value() : nullptr;
        }
        [[nodiscard]] Buffer* spot_shadow_matrices_buffer()
        {
            return spot_shadow_matrices_buffer_ ? &spot_shadow_matrices_buffer_.value() : nullptr;
        }
        [[nodiscard]] std::uint32_t point_shadow_map_size() const { return point_shadow_map_size_; }
        [[nodiscard]] bool point_shadow_enabled() const { return point_shadow_enabled_; }
        [[nodiscard]] std::span<const glm::mat4> point_shadow_view_projections() const
        {
            return point_shadow_view_projections_;
        }
        [[nodiscard]] Texture* spot_shadow_map() const { return spot_shadow_map_; }
        [[nodiscard]] Sampler* spot_shadow_sampler() const { return spot_shadow_sampler_; }
        [[nodiscard]] std::uint32_t spot_shadow_map_size() const { return spot_shadow_map_size_; }
        [[nodiscard]] bool spot_shadow_enabled() const { return spot_shadow_enabled_; }
        [[nodiscard]] std::span<const glm::mat4> spot_shadow_view_projections() const
        {
            return spot_shadow_view_projections_;
        }

    private:
        MaterialLoader() = default;

        static std::optional<MaterialDefinition> load_material_definition(const fs::path& path);

        void destroy(std::string_view name);

        static Material create(
            std::string_view name,
            const MaterialSettings& settings);

        void ensure_cluster_buffers(const glm::uvec3& cluster_grid_size);
        void setup(Material* material, const MaterialDefinition& def);

        rhi::Device*         device_{ nullptr };
        rhi::Swapchain*      swapchain_{ nullptr };
        rhi::DescriptorPool* descriptor_pool_{ nullptr };
        rhi::ResourceCache*  resource_cache_{ nullptr };
        rhi::GraphicsApi     api_{};

        flat_map<std::string, MaterialDefinition> definitions_;
        ResourceRegistry<Material> materials_;

        std::optional<Buffer> camera_ubo_;
        std::optional<Buffer> lighting_ubo_;
        std::optional<Buffer> point_lights_buffer_;
        std::optional<Buffer> directional_lights_buffer_;
        std::optional<Buffer> spot_lights_buffer_;
        std::optional<Buffer> cluster_records_buffer_;
        std::optional<Buffer> cluster_light_indices_buffer_;
        std::optional<Buffer> cluster_light_counter_buffer_;
        std::optional<Buffer> shadow_casters_buffer_;
        std::optional<Buffer> time_ubo_;
        std::optional<Buffer> point_shadow_matrices_buffer_;
        std::optional<Buffer> spot_shadow_matrices_buffer_;

        Texture* directional_shadow_map_{ nullptr };
        Sampler* directional_shadow_sampler_{ nullptr };
        std::uint32_t directional_shadow_map_size_{ 2048u };
        bool directional_shadow_enabled_{ false };

        Texture* point_shadow_map_{ nullptr };
        Sampler* point_shadow_sampler_{ nullptr };
        std::uint32_t point_shadow_map_size_{ 1024u };
        bool point_shadow_enabled_{ false };

        Texture* spot_shadow_map_{ nullptr };
        Sampler* spot_shadow_sampler_{ nullptr };
        std::uint32_t spot_shadow_map_size_{ 1024u };
        bool spot_shadow_enabled_{ false };

        Texture* frame_depth_texture_{ nullptr };
        Texture* ssao_texture_{ nullptr };
        Sampler* renderer_sampler_{ nullptr };
        std::uint32_t frame_render_target_width_{ 0 };
        std::uint32_t frame_render_target_height_{ 0 };
        std::uint64_t frame_render_target_revision_{ 0 };

        struct PointShadowSource final
        {
            std::uint32_t slot{ 0u };
            glm::vec3 position{ 0.0f };
            float range{ 1.0f };
        };

        struct SpotShadowSource final
        {
            std::uint32_t slot{ 0u };
            glm::vec3 position{ 0.0f };
            glm::vec3 direction{ 0.0f, 0.0f, 1.0f };
            float outer_angle{ glm::radians(32.0f) };
            float range{ 1.0f };
        };

        LightingUBO lighting_data_{};
        glm::uvec3 cluster_grid_size_{ 1u, 1u, 1u };
        std::size_t cluster_record_capacity_{ 0 };
        std::size_t cluster_light_index_capacity_{ 0 };
        glm::mat4 light_view_{ 1.0f };
        glm::mat4 light_projection_{ 1.0f };
        std::vector<GpuPointLight> point_lights_;
        std::vector<GpuDirectionalLight> directional_lights_;
        std::vector<GpuSpotLight> spot_lights_;
        std::vector<GpuClusterRecord> cluster_records_;
        std::vector<std::uint32_t> cluster_light_indices_;
        std::vector<GpuShadowCaster> shadow_casters_;
        std::vector<PointShadowSource> point_shadow_sources_;
        std::vector<SpotShadowSource> spot_shadow_sources_;
        std::vector<glm::mat4> point_shadow_view_projections_;
        std::vector<glm::mat4> spot_shadow_view_projections_;

        bool initialized_{ false };

        friend class Material;
    };
}

