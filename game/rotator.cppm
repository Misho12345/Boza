module game:rotator;

import std;
import boza;

using namespace boza;

namespace game
{
    class Rotator final : public Behaviour
    {
    public:
        float     rotation_speed{ 1.0f };
        glm::vec3 rotation_axis{ 0.0f, 1.0f, 0.0f };

        void update(const float dt) override
        {
            const glm::quat delta_rot = glm::angleAxis(rotation_speed * dt, glm::normalize(rotation_axis));
            transform->rotation       = delta_rot * static_cast<glm::quat>(transform->rotation);
        }
    };
}
