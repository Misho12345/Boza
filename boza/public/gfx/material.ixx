module;

#include "api.hpp"

export module boza.gfx:material;

import std;
import boza.common;

export namespace boza
{
    class Material;
    class Texture;
    class Buffer;

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

        template<typename T>
        PropertyBinder& operator=(const T& value);

        PropertyBinder& operator=(Texture* texture);
        PropertyBinder& operator=(Buffer* buffer);

    private:
        Material*   material_;
        std::string name_;
    };

    class BOZA_API Material
    {
    public:
        static Material* create(const std::string& vertex_shader_name, const std::string& fragment_shader_name);
        static Material* get(const std::string& name);

        ~Material();

        PropertyBinder operator[](std::string_view name);

        void bind() const;

        template<typename T>
        void push_constants(const std::string& name, const T& value)
        {
            push_constants_impl(name, &value, sizeof(T));
        }

        template<typename T>
        void update_property(const std::string& name, const T& value)
        {
            update_property_impl(name, &value, sizeof(T));
        }

        void update_texture(const std::string& name, const Texture* texture);
        void update_buffer(const std::string& name, const Buffer* buffer);

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
        void push_constants_impl(const std::string& name, const void* data, std::size_t size) const;
        void update_property_impl(const std::string& name, const void* data, std::size_t size) const;

        friend class PropertyBinder;
    };

    template<typename T>
    PropertyBinder& PropertyBinder::operator=(const T& value)
    {
        material_->update_property<T>(name_, value);
        return *this;
    }
}
