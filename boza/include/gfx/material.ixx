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

        ~Material();

        PropertyBinder operator[](std::string_view name);

        void bind();

        template<typename T>
        void push_constants(const std::string& name, const T& value);

        template<typename T>
        void update_property(const std::string& name, const T& value);

        void update_texture(const std::string& name, Texture* texture);
        void update_buffer(const std::string& name, Buffer* buffer);

        [[nodiscard]] void* rhi_pipeline_handle() const;
        [[nodiscard]] void* rhi_pipeline_layout_handle() const;

        [[nodiscard]] std::size_t descriptor_set_count() const;
        [[nodiscard]] void*       rhi_descriptor_set_handle(std::size_t index) const;

    private:
        Material();

        struct Impl;
        std::unique_ptr<Impl> impl_;

        void mark_set_dirty(std::uint32_t set);

        friend class PropertyBinder;
    };

    template<typename T>
    PropertyBinder& PropertyBinder::operator=(const T& value)
    {
        material_->update_property<T>(name_, value);
        return *this;
    }
}
