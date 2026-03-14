module;

#include "api.hpp"

export module boza.gfx:material;

import std;
import boza.common;
import :common;

namespace boza::gfx
{
    class MaterialLoader;
}

namespace boza
{
    class Material;

    struct MaterialAccess
    {
        static void* pipeline(const Material& material);
        static void* pipeline_layout(const Material& material);
        static const void* reflection(const Material& material);
        static std::span<const std::uint8_t> push_constant_staging(const Material& material);
        static void bind_descriptor_sets(const Material& material);
    };
}

export namespace boza
{
    class Material;
    class Texture;
    class Buffer;
    class Sampler;

    enum class CompareOp : std::uint8_t
    {
        Never,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum class CullMode : std::uint8_t
    {
        None,
        Front,
        Back,
        FrontAndBack
    };

    enum class FrontFace : std::uint8_t
    {
        CounterClockwise,
        Clockwise
    };

    struct MaterialSettings
    {
        std::string vertex_shader;
        std::string fragment_shader;
        CompareOp   depth_compare_op{ CompareOp::Less };
        bool        depth_test_enable{ true };
        bool        depth_write_enable{ true };
        CullMode    cull_mode{ CullMode::Back };
        FrontFace   front_face{ FrontFace::CounterClockwise };

        bool operator==(const MaterialSettings&) const = default;

        [[nodiscard]] std::size_t hash() const noexcept
        {
            std::size_t h = 0;

            h ^= std::hash<std::string>{}(vertex_shader);
            h ^= std::hash<std::string>{}(fragment_shader) << 1;
            h ^= std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(depth_compare_op)) << 2;
            h ^= std::hash<bool>{}(depth_test_enable) << 8;
            h ^= std::hash<bool>{}(depth_write_enable) << 9;
            h ^= std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(cull_mode)) << 10;
            h ^= std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(front_face)) << 14;
            return h;
        }
    };

    struct BindingInfo
    {
        std::uint32_t set{ 0 };
        std::uint32_t binding{ 0 };
        std::uint32_t offset{ 0 };
        std::uint32_t size{ 0 };
        std::uint32_t descriptor_type{ 0 };
        std::uint32_t data_type{ 0 };
        bool          is_push_constant{ false };
    };

    class BOZA_API PropertyBinder
    {
    public:
        PropertyBinder(Material* material, std::string name)
            : material_(material),
              name_(std::move(name)) {}

        template <typename T> requires requires
        {
            requires !std::same_as<std::remove_cvref_t<T>, Texture>;
            requires !std::same_as<std::remove_cvref_t<T>, Buffer>;
            requires !std::same_as<std::remove_cvref_t<T>, std::pair<Texture&, Sampler&>>;
        }
        PropertyBinder& operator=(const T& value);

        PropertyBinder& operator=(const Texture& texture);
        PropertyBinder& operator=(const Buffer& buffer);
        PropertyBinder& operator=(std::pair<Texture&, Sampler&> texture_sampler);

    private:
        Material*   material_;
        std::string name_;
    };

    class BOZA_API Material
    {
    public:
        static Material& create(
            std::string_view        name,
            const MaterialSettings& settings);

        static Material& get(std::string_view name);
        static Material* try_get(std::string_view name);
        static bool exists(const Material* ptr);

        static void destroy(std::string_view name);

        ~Material();

        Material(const Material&)            = delete;
        Material& operator=(const Material&) = delete;
        Material(Material&&) noexcept;
        Material& operator=(Material&&) noexcept;

        PropertyBinder operator[](std::string_view name);

        void bind() const;

        template <typename T>
        void push_constants(const std::string& name, const T& value)
        {
            push_constants(name, &value, sizeof(T), get_shader_data_type<T>());
        }

        template <typename T>
        void update_property(const std::string& name, const T& value)
        {
            update_property(name, &value, sizeof(T), get_shader_data_type<T>());
        }

        void update_texture(const std::string& name, const Texture& texture);
        void update_texture(const std::string& name, const Texture& texture, Sampler& sampler);
        void update_buffer(const std::string& name, const Buffer& buffer);

        [[nodiscard]] std::optional<BindingInfo> lookup_binding(const std::string& name) const;

        [[nodiscard]] std::size_t        descriptor_set_count() const;
        [[nodiscard]] const std::string& name() const { return name_; }

        [[nodiscard]]
        bool cpu_cull_enabled() const { return cpu_cull_enabled_; }
        void set_cpu_cull_enabled(const bool enabled) { cpu_cull_enabled_ = enabled; }

    private:
        using RhiOwnedHandle = std::unique_ptr<void, void(*)(void*)>;

        static void destroy_rhi_buffer(void* handle);
        static void destroy_reflection(void* handle);

        explicit Material(std::string_view name);
        void     cleanup();

        std::string name_;

        void*                     pipeline_{ nullptr };
        void*                     pipeline_layout_{ nullptr };
        std::vector<void*>        descriptor_set_layouts_;
        std::vector<void*>        descriptor_sets_;
        std::vector<std::vector<void*>> descriptor_sets_per_frame_;
        std::vector<std::uint8_t> push_constant_staging_;

        RhiOwnedHandle reflection_{ nullptr, &Material::destroy_reflection };

        flat_map<std::uint32_t, bool> dirty_sets_;

        flat_map<std::uint32_t, std::vector<std::uint8_t>>   uniform_buffer_staging_;
        flat_map<std::uint32_t, std::vector<RhiOwnedHandle>> uniform_buffers_;
        flat_map<std::uint32_t, std::vector<void*>>          bound_buffer_handles_;
        flat_map<std::uint32_t, std::vector<void*>>          bound_texture_handles_;
        flat_map<std::uint32_t, std::vector<void*>>          bound_sampler_handles_;

        flat_map<std::string, Sampler*> default_samplers_;

        void* descriptor_pool_{ nullptr };
        bool  cpu_cull_enabled_{ true };

        [[nodiscard]] void* rhi_pipeline_layout_handle() const { return pipeline_layout_; }
        [[nodiscard]] void* rhi_pipeline_handle() const { return pipeline_; }
        [[nodiscard]] const void* reflection_handle() const { return reflection_.get(); }
        [[nodiscard]] std::span<const std::uint8_t> push_constant_staging() const { return push_constant_staging_; }

        void mark_set_dirty(std::uint32_t set);
        void bind_descriptor_sets() const;

        void push_constants(
            const std::string& name,
            const void*        data,
            std::size_t        size,
            ShaderDataType     type) const;

        void update_property(
            const std::string& name,
            const void*        data,
            std::size_t        size,
            ShaderDataType     type);

        friend class PropertyBinder;
        friend struct MaterialAccess;
        friend class gfx::MaterialLoader;
        friend struct RenderingSystem;
    };

    template <typename T> requires requires
    {
        requires !std::same_as<std::remove_cvref_t<T>, Texture>;
        requires !std::same_as<std::remove_cvref_t<T>, Buffer>;
        requires !std::same_as<std::remove_cvref_t<T>, std::pair<Texture&, Sampler&>>;
    }
    PropertyBinder& PropertyBinder::operator=(const T& value)
    {
        material_->update_property(name_, &value, sizeof(T), get_shader_data_type<T>());
        return *this;
    }
}
