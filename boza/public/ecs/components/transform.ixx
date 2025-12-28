module;

#include "api.hpp"
#include <cstddef>

export module boza.ecs:transform;

import std;
import boza.common;
import :component;

export namespace boza
{
    class BOZA_API Transform final : public Component
    {
    public:
        Transform()           = default;
        ~Transform() override = default;

        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::quat rotation{ glm::identity<glm::quat>() };
        glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

        PropertyGetSet<Transform, glm::vec3> eulers
        {
            &Transform::get_eulers,
            &Transform::set_eulers,
            offsetof(Transform, eulers)
        };

        PropertyGet<Transform, glm::vec3> forward{ &Transform::get_forward, offsetof(Transform, forward) };
        PropertyGet<Transform, glm::vec3> right{ &Transform::get_right, offsetof(Transform, right) };
        PropertyGet<Transform, glm::vec3> up{ &Transform::get_up, offsetof(Transform, up) };

        [[nodiscard]] glm::mat4 model_matrix() const;
        [[nodiscard]] glm::mat4 view_matrix() const;

        void look_at(const glm::vec3& target, const glm::vec3& world_up = glm::vec3(0.0f, 1.0f, 0.0f))
        {
            const glm::vec3 direction = normalize(target - position);
            rotation = glm::quatLookAtLH(direction, world_up);
        }

    private:
        [[nodiscard]] glm::vec3 get_forward() const { return glm::rotate(rotation, glm::vec3(0.0f, 0.0f, -1.0f)); }
        [[nodiscard]] glm::vec3 get_right() const { return glm::rotate(rotation, glm::vec3(1.0f, 0.0f, 0.0f)); }
        [[nodiscard]] glm::vec3 get_up() const { return glm::rotate(rotation, glm::vec3(0.0f, 1.0f, 0.0f)); }

        [[nodiscard]]
        glm::vec3 get_eulers() const { return glm::eulerAngles(rotation); }
        void set_eulers(const glm::vec3& value) { rotation = glm::quat(value); }
    };
}
