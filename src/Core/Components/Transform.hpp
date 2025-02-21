#pragma once
#include "Component.hpp"
#include "boza_pch.hpp"

namespace boza
{
    struct BOZA_API Transform final : Component
    {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;

        Transform(
            const glm::vec3& position,
            const glm::vec3& rotation,
            const glm::vec3& scale)
            : position{ position },
              rotation{ rotation },
              scale{ scale } {}
    };
}
