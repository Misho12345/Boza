#include "boza/core/Rigidbody.hpp"
#include "boza/core/GameObject.hpp"
#include "boza/core/Time.hpp"
#include "boza/core/Transform.hpp"

namespace boza
{
    void Rigidbody::fixed_update()
    {
        if (is_kinematic) return;

        if (use_gravity) velocity += glm::vec3(0.0f, -9.81f, 0.0f) * Time::fixed_delta_time();

        transform->position += velocity * Time::fixed_delta_time();
        transform->rotation = glm::normalize(glm::quat(angular_velocity * Time::fixed_delta_time()) * transform->rotation);

        velocity *= 1.0f - drag;
        angular_velocity *= 1.0f - angular_drag;
    }
}
