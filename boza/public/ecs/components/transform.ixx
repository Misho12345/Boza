module;

#include "api.hpp"

export module boza.ecs:transform;

import std;
import boza.common;
import :component;
import :scene;

export namespace boza
{
    enum class ParentChangeStrategy : std::uint8_t
    {
        KeepLocal,
        KeepWorld
    };

    class BOZA_API Transform final : public Component
    {
        [[nodiscard]] glm::vec3 get_position() const { return world_position_; }
        [[nodiscard]] glm::quat get_rotation() const { return world_rotation_; }
        [[nodiscard]] glm::vec3 get_scale() const { return world_scale_; }
        [[nodiscard]] glm::vec3 get_eulers() const { return glm::eulerAngles(world_rotation_); }

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


        [[nodiscard]] Transform*       get_parent_ptr() { return parent_; }
        [[nodiscard]] const Transform* get_parent_cptr() const { return parent_; }

    public:
        Transform()           = default;
        ~Transform() override = default;

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


        [[msvc::no_unique_address]] Property<Transform, &Transform::get_forward > forward{ this };
        [[msvc::no_unique_address]] Property<Transform, &Transform::get_right> right{ this };
        [[msvc::no_unique_address]] Property<Transform, &Transform::get_up> up{ this };

        [[msvc::no_unique_address]]
        Property<
            Transform,
            &Transform::get_parent_ptr,
            &Transform::get_parent_cptr
        > parent{ this };

        [[nodiscard]] glm::mat4 world_matrix() const { return world_matrix_; }
        [[nodiscard]] glm::mat4 local_matrix() const;
        [[nodiscard]] glm::mat4 view_matrix() const;

        void look_at(
            const glm::vec3& target,
            const glm::vec3& world_up = glm::vec3{ 0.0f, 1.0f, 0.0f }
        );

        void set_parent(
            Transform* new_parent,
            ParentChangeStrategy strategy = ParentChangeStrategy::KeepWorld
        );

        [[nodiscard]] bool has_parent() const { return parent_ != nullptr; }

        [[nodiscard]] std::vector<Transform*> get_children() const;
        void for_each_child(const std::function<void(Transform&)>& callback) const;

        void evaluate_world_transform();

        void on_clone(GameObject& target) override;

    private:
        void mark_dirty();
        void mark_children_dirty() const;

        Transform*              parent_{ nullptr };
        std::vector<Transform*> children_{};

        glm::vec3 local_position_{ 0.0f };
        glm::quat local_rotation_{ glm::identity<glm::quat>() };
        glm::vec3 local_scale_{ 1.0f };

        glm::vec3 world_position_{ 0.0f };
        glm::quat world_rotation_{ glm::identity<glm::quat>() };
        glm::vec3 world_scale_{ 1.0f };
        glm::mat4 world_matrix_{ 1.0f };

        bool is_dirty_{ true };

        friend class Scene;
    };
}
