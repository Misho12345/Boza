#pragma once
#include "boza/API.hpp"
#include "boza/core/Component.hpp"
#include "boza/core/Property.hpp"
#include <memory>
#include <string>

namespace boza
{
    class Mesh;
    class Material;

    #ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251)
    #endif

    class BOZA_API MeshRenderer final : public Component
    {
    public:
        std::shared_ptr<Mesh> mesh;
        std::string           material_name = "default";

        PropertyGet<Material*> material{ GET -> Material* { return get_material(); } };

        Material* create_material_instance(const std::string& instance_name, const std::string& base_material = "");

    private:
        Material* get_material() const;
    };

    #ifdef _MSC_VER
    #pragma warning(pop)
    #endif
}
