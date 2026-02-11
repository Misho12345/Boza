module;

#include "api.hpp"

export module boza.ecs:camera;

import boza.common;
import <flecs.h>;

export namespace boza
{
    class BOZA_API Camera final
    {
        [[nodiscard]]
        bool get_is_primary() const { return is_primary_; }
        void set_is_primary(bool value);

    public:
        enum class ProjectionType : std::uint8_t
        {
            Perspective,
            Orthographic
        };

        ProjectionType projection_type{ ProjectionType::Perspective };
        float fov{ 45.0f };
        float near_clip{ 0.1f };
        float far_clip{ 1000.0f };
        float ortho_size{ 10.0f };

        [[msvc::no_unique_address]]
        Property<
            Camera,
            &Camera::get_is_primary,
            &Camera::set_is_primary
        > is_primary{ this };

        [[nodiscard]]
        glm::mat4 projection_matrix(float aspect_ratio) const;

    private:
        bool is_primary_{ false };
        flecs::entity entity_{};

        friend class GameObject;
    };
}
