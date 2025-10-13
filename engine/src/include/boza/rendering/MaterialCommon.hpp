#pragma once
#include "boza/API.hpp"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <variant>

namespace boza
{
    enum class MaterialLoadStrategy
    {
        GameLoad,
        SceneInit,
        OnDemand
    };

    using MaterialPropertyValue = std::variant<
        float,
        glm::vec2,
        glm::vec3,
        glm::vec4,
        int,
        glm::ivec2,
        glm::ivec3,
        glm::ivec4
    >;

    struct BOZA_API MaterialDefinition
    {
        #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4251)
        #endif

        std::string base_material;

        std::string vertex_shader;
        std::string fragment_shader;

        MaterialLoadStrategy load_strategy{ MaterialLoadStrategy::OnDemand };

        std::unordered_map<std::string, std::string>           textures;
        std::unordered_map<std::string, MaterialPropertyValue> properties;

        bool is_mutable{ true };

        #ifdef _MSC_VER
        #pragma warning(pop)
        #endif
    };
}
