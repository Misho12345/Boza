#pragma once

#include "boza/API.hpp"
#include "boza/core/Property.hpp"
#include "MaterialCommon.hpp"
#include <memory>
#include <string>

namespace boza
{
    enum class GraphicsApi;
    class Texture;

    class BOZA_API Material
    {
    public:
        Material(const std::string& name, const MaterialDefinition& def, class MaterialSystem* system);
        ~Material();

        Material(const Material&)            = delete;
        Material& operator=(const Material&) = delete;

        Material(Material&&) noexcept;
        Material& operator=(Material&&) noexcept;

        bool initialize() const;
        void destroy() const;

        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4251)
        #endif

        PropertyGet<const std::string&> name{ GET -> const std::string& { return get_name(); } };

        PropertyGet<const MaterialDefinition&> definition
        {
            GET -> const MaterialDefinition& { return get_definition(); }
        };

        PropertyGet<bool> is_mutable{ GET { return get_is_mutable(); } };
        PropertyGet<bool> is_dirty{ GET { return get_is_dirty(); } };

        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif

        [[nodiscard]] Texture* get_texture(const std::string& name_) const;

        bool set_property(const std::string& name_, const MaterialPropertyValue& value) const;

        MaterialPropertyValue& operator[](const std::string& name_) const;

        void upload_to_gpu(uint32_t frame_index = 0) const;
        void mark_dirty() const;

        void* get_material_buffer_internal() const;

    private:
        [[nodiscard]] const std::string&        get_name() const;
        [[nodiscard]] const MaterialDefinition& get_definition() const;

        [[nodiscard]] bool get_is_mutable() const;
        [[nodiscard]] bool get_is_dirty() const;

        struct Impl;
        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4251)
        #endif
        std::unique_ptr<Impl> impl_;
        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif

        friend class MaterialSystem;
    };
}
