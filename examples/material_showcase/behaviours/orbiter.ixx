export module behaviours:orbiter;

import std;
import boza;

using namespace boza;

export class Orbiter final : public Behaviour
{
public:
    explicit Orbiter(
        const glm::vec3 axis                 = glm::vec3{ 0.0f, 1.0f, 0.0f },
        const float     distance_from_target = 5.0f,
        const float     orbit_speed          = 50.0f,
        const float     initial_angle        = 0.0f)
        : axis{ axis },
          distance_from_target{ distance_from_target },
          orbit_speed{ orbit_speed },
          angle{ initial_angle } {}

    glm::vec3 axis{ 0.0f, 1.0f, 0.0f };

    float distance_from_target{ 5.0f };
    float orbit_speed{ 50.0f };
    float angle{ 0.0f };


    void start() override { transform->local_position = get_local_position_on_orbit(glm::radians(angle)); }

    void update() override
    {
        if (!transform->has_parent()) return;

        angle += orbit_speed * Time::delta_time();

        transform->local_position = get_local_position_on_orbit(glm::radians(angle));
        transform->look_at(transform->parent->position());
    }

private:
    glm::vec3 get_local_position_on_orbit(const float t) const
    {
        const glm::vec3 n = normalize(axis);

        const glm::vec3 helper = (glm::abs(n.y) < 0.999f) ? glm::vec3{ 0, 1, 0 } : glm::vec3{ 1, 0, 0 };

        const glm::vec3 u = normalize(cross(helper, n));
        const glm::vec3 v = cross(n, u);

        return (glm::cos(t) * u + glm::sin(t) * v) * distance_from_target;
    }
};
