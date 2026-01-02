module;

#include "api.hpp"

export module boza.gfx:material;

import std;
import boza.common;
import :common;

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
        CompareOp depth_compare_op{ CompareOp::Less };
        bool      depth_test_enable{ true };
        bool      depth_write_enable{ true };
        CullMode  cull_mode{ CullMode::Back };
        FrontFace front_face{ FrontFace::CounterClockwise };

        bool operator==(const MaterialSettings&) const = default;

        [[nodiscard]] std::size_t hash() const noexcept
        {
            std::size_t h = 0;
            h ^= std::hash<std::uint8_t>{}(static_cast<std::uint8_t>(depth_compare_op)) << 0;
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
        bool is_push_constant{ false };
    };

    class BOZA_API PropertyBinder
    {
    public:
        PropertyBinder(Material* material, std::string name)
            : material_(material),
              name_(std::move(name)) {}

        template<typename T> requires requires
        {
            requires !std::is_same_v<std::remove_cvref_t<T>, Texture*>;
            requires !std::is_same_v<std::remove_cvref_t<T>, Buffer*>;
            requires !std::is_same_v<std::remove_cvref_t<T>, std::pair<Texture*, Sampler*>>;
        }
        PropertyBinder& operator=(const T& value);

        PropertyBinder& operator=(const Texture* texture);
        PropertyBinder& operator=(const Buffer* buffer);
        PropertyBinder& operator=(const std::pair<Texture*, Sampler*>& texture_sampler);

    private:
        Material*   material_;
        std::string name_;
    };

    class BOZA_API Material
    {
    public:
        static Material* create(
            const std::string& vertex_shader_name,
            const std::string& fragment_shader_name,
            const MaterialSettings& settings = {});
        static Material* get(const std::string& name);

        ~Material();

        PropertyBinder operator[](std::string_view name);

        void bind() const;

        template<typename T>
        void push_constants(const std::string& name, const T& value)
        {
            push_constants_impl(name, &value, sizeof(T), get_shader_data_type<T>());
        }

        template<typename T>
        void update_property(const std::string& name, const T& value)
        {
            update_property_impl(name, &value, sizeof(T), get_shader_data_type<T>());
        }

        void update_texture(const std::string& name, const Texture* texture, const Sampler* sampler = nullptr) const;
        void update_texture_sampler(const std::string& name, const Texture* texture, const Sampler* sampler) const;
        void update_buffer(const std::string& name, const Buffer* buffer) const;

        [[nodiscard]] std::optional<BindingInfo> lookup_binding(const std::string& name) const;

        [[nodiscard]] void* rhi_pipeline_handle() const;
        [[nodiscard]] void* rhi_pipeline_layout_handle() const;

        [[nodiscard]] std::size_t descriptor_set_count() const;
        [[nodiscard]] void*       rhi_descriptor_set_handle(std::size_t index) const;

    private:
        Material();

        struct Impl;
        std::unique_ptr<Impl> impl_;

        void mark_set_dirty(std::uint32_t set) const;

        void push_constants_impl(const std::string& name, const void* data, std::size_t size, ShaderDataType type) const;
        void update_property_impl(const std::string& name, const void* data, std::size_t size, ShaderDataType type) const;

        friend class PropertyBinder;
    };

    template<typename T> requires requires
    {
        requires !std::is_same_v<std::remove_cvref_t<T>, Texture*>;
        requires !std::is_same_v<std::remove_cvref_t<T>, Buffer*>;
        requires !std::is_same_v<std::remove_cvref_t<T>, std::pair<Texture*, Sampler*>>;
    }
    PropertyBinder& PropertyBinder::operator=(const T& value)
    {
        material_->update_property_impl(name_, &value, sizeof(T), get_shader_data_type<T>());
        return *this;
    }
}