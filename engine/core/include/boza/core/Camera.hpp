#pragma once
#include "boza/pch.hpp"
#include "Component.hpp"

namespace boza
{
    class Camera final : public Component
    {
    public:
        enum class ProjectionType { Perspective, Orthographic };

        ProjectionType projection_type{ ProjectionType::Perspective };

        float fov{ 45.0f };
        float near_clip{ 0.1f };
        float far_clip{ 1000.0f };

        float ortho_size{ 10.0f };

        bool primary{ true };

        glm::mat4 projection_matrix(float aspect_ratio) const;
    };
}
