module;

#include "api.hpp"
#include <cstddef>

export module boza.ecs:transform;

import std;
import boza.common;
import :component;
import :scene;

import <entt/entt.hpp>;

export namespace boza
{
    class BOZA_API Transform final : public Component
    {
    public:
        Transform()           = default;
        ~Transform() override = default;

        PropertyGetSet<Transform, glm::vec3> position
        {
            &Transform::get_position,
            &Transform::set_position,
            offsetof(Transform, position)
        };

        PropertyGetSet<Transform, glm::quat> rotation
        {
            &Transform::get_rotation,
            &Transform::set_rotation,
            offsetof(Transform, rotation)
        };

        PropertyGetSet<Transform, glm::vec3> scale
        {
            &Transform::get_scale,
            &Transform::set_scale,
            offsetof(Transform, scale)
        };

        PropertyGetSet<Transform, glm::vec3> eulers
        {
            &Transform::get_eulers,
            &Transform::set_eulers,
            offsetof(Transform, eulers)
        };


        PropertyGetSet<Transform, glm::vec3> local_position
        {
            &Transform::get_local_position,
            &Transform::set_local_position,
            offsetof(Transform, local_position)
        };

        PropertyGetSet<Transform, glm::quat> local_rotation
        {
            &Transform::get_local_rotation,
            &Transform::set_local_rotation,
            offsetof(Transform, local_rotation)
        };

        PropertyGetSet<Transform, glm::vec3> local_scale
        {
            &Transform::get_local_scale,
            &Transform::set_local_scale,
            offsetof(Transform, local_scale)
        };

        PropertyGetSet<Transform, glm::vec3> local_eulers
        {
            &Transform::get_local_eulers,
            &Transform::set_local_eulers,
            offsetof(Transform, local_eulers)
        };

        PropertyGet<Transform, glm::vec3> forward{ &Transform::get_forward, offsetof(Transform, forward) };
        PropertyGet<Transform, glm::vec3> right{ &Transform::get_right, offsetof(Transform, right) };
        PropertyGet<Transform, glm::vec3> up{ &Transform::get_up, offsetof(Transform, up) };

        PropertyGetSet<Transform, Transform&> parent
        {
            &Transform::get_parent,
            &Transform::set_parent,
            offsetof(Transform, parent)
        };

        [[nodiscard]] glm::mat4 model_matrix() const;
        [[nodiscard]] glm::mat4 view_matrix() const;

        void look_at(const glm::vec3& target, const glm::vec3& world_up = glm::vec3(0.0f, 1.0f, 0.0f));

        [[nodiscard]] bool has_parent() const { return parent_ != entt::null; }

        [[nodiscard]]
        std::vector<Transform*> get_children() const;

        void for_each_child(const std::function<void(Transform&)>& callback) const;

    private:
        [[nodiscard]] glm::vec3 get_position() const;
        [[nodiscard]] glm::quat get_rotation() const;
        [[nodiscard]] glm::vec3 get_scale() const;
        [[nodiscard]] glm::vec3 get_eulers() const;

        [[nodiscard]] glm::vec3 get_local_position() const;
        [[nodiscard]] glm::quat get_local_rotation() const;
        [[nodiscard]] glm::vec3 get_local_scale() const;
        [[nodiscard]] glm::vec3 get_local_eulers() const;

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

        [[nodiscard]] Transform& get_parent() const;
        void set_parent(Transform& new_parent);

        [[nodiscard]]
        Transform* get_parent_transform() const;

        void mark_dirty() const;
        void update_cached_matrix() const;

        entt::entity              parent_{ entt::null };
        std::vector<entt::entity> children_{};

        glm::vec3 position_{ 0.0f, 0.0f, 0.0f };
        glm::quat rotation_{ glm::identity<glm::quat>() };
        glm::vec3 scale_{ 1.0f, 1.0f, 1.0f };

        mutable glm::mat4 cached_model_matrix_{ 1.0f };
        mutable bool      is_dirty_{ true };
    };
}
