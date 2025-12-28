export module behaviours:rotator;

import std;
import boza;

using namespace boza;

export class Rotator final : public Behaviour
{
public:
    float     rotation_speed{ 1.0f };
    glm::vec3 rotation_axis{ 0.0f, 1.0f, 0.0f };

    void update(const float dt) override
    {
        const glm::quat delta_rot = glm::angleAxis(rotation_speed * dt, normalize(rotation_axis));
        transform->rotation       = delta_rot * transform->rotation;
    }
};
