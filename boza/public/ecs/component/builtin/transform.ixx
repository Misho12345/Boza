module;

#include "api.hpp"

export module boza.ecs:transform;

import std;
import boza.common;
import <flecs.h>;

export namespace boza
{
    class BOZA_API Transform final
    {
        [[nodiscard]] glm::vec3 get_position() const;
        [[nodiscard]] glm::quat get_rotation() const;
        [[nodiscard]] glm::vec3 get_scale() const;
        [[nodiscard]] glm::vec3 get_eulers() const;

        [[nodiscard]] glm::vec3 get_local_position() const { return local_position_; }
        [[nodiscard]] glm::quat get_local_rotation() const { return local_rotation_; }
        [[nodiscard]] glm::vec3 get_local_scale() const { return local_scale_; }
        [[nodiscard]] glm::vec3 get_local_eulers() const { return glm::eulerAngles(local_rotation_); }

        void set_position(const glm::vec3& value);
        void set_rotation(const glm::quat& value);
        void set_scale(const glm::vec3& value);
        void set_eulers(const glm::vec3& value);

        void set_local_position(const glm::vec3& value);
        void set_local_rotation(const glm::quat& value);
        void set_local_scale(const glm::vec3& value);
        void set_local_eulers(const glm::vec3& value);

        [[nodiscard]] glm::vec3 get_forward() const;
        [[nodiscard]] glm::vec3 get_right() const;
        [[nodiscard]] glm::vec3 get_up() const;

    public:
        Transform(
            const glm::vec3 local_position = glm::vec3{ 0.0f },
            const glm::quat local_rotation = glm::identity<glm::quat>(),
            const glm::vec3 local_scale    = glm::vec3{ 1.0f })
            : local_position_{ local_position },
              local_rotation_{ local_rotation },
              local_scale_{ local_scale },
              world_position_{ local_position },
              world_rotation_{ local_rotation },
              world_scale_{ local_scale } { world_matrix_ = local_matrix(); }

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_position,
            &Transform::set_position
        > position{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_rotation,
            &Transform::set_rotation
        > rotation{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_scale,
            &Transform::set_scale
        > scale{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_eulers,
            &Transform::set_eulers
        > eulers{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_local_position,
            &Transform::set_local_position
        > local_position{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_local_rotation,
            &Transform::set_local_rotation
        > local_rotation{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_local_scale,
            &Transform::set_local_scale
        > local_scale{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_local_eulers,
            &Transform::set_local_eulers
        > local_eulers{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_forward
        > forward{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_right
        > right{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_up
        > up{ this };

        [[nodiscard]] glm::mat4 world_matrix() const;
        [[nodiscard]] glm::mat4 local_matrix() const;
        [[nodiscard]] glm::mat4 view_matrix() const;

        void look_at(
            const glm::vec3& target,
            const glm::vec3& world_up = glm::vec3{ 0.0f, 1.0f, 0.0f }
        );

    private:
        void evaluate_world_transform();
        void mark_dirty() const;
        void ensure_world_transform_up_to_date() const;

        glm::vec3 local_position_{ 0.0f };
        glm::quat local_rotation_{ glm::identity<glm::quat>() };
        glm::vec3 local_scale_{ 1.0f };

        glm::vec3 world_position_{ 0.0f };
        glm::quat world_rotation_{ glm::identity<glm::quat>() };
        glm::vec3 world_scale_{ 1.0f };
        glm::mat4 world_matrix_{ glm::identity<glm::mat4>() };
        bool dirty_{ false };
        std::uint64_t world_revision_{ 1 };
        std::uint64_t parent_world_revision_{ 0 };
        std::uint64_t last_ensure_frame_{ 0 };

        flecs::entity entity_{};

        friend class GameObject;
        friend struct TransformSystem;
        friend struct RenderingSystem;
    };
}
