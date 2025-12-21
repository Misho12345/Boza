module;

#include "api.hpp"

export module boza.ecs:camera;

import boza.common;
import :component;

export namespace boza
{
    class BOZA_API Camera final : public Component
    {
    public:
        enum class ProjectionType { Perspective, Orthographic };

        ProjectionType projection_type{ ProjectionType::Perspective };
        float fov{ 45.0f };
        float near_clip{ 0.1f };
        float far_clip{ 1000.0f };
        float ortho_size{ 10.0f };
        bool primary{ true };

        [[nodiscard]] glm::mat4 projection_matrix(float aspect_ratio) const;
    };
}