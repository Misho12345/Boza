export module behaviours:rotator;

import std;
import boza;

using namespace boza;

export class Rotator final : public Behaviour
{
public:
    float     rotation_speed{ 1.0f };
    glm::vec3 rotation_axis{ 0.0f, 1.0f, 0.0f };

    void update() override
    {
        const glm::quat delta_rot = glm::angleAxis(rotation_speed * Time::delta_time, normalize(rotation_axis));
        transform->local_rotation = normalize(delta_rot * transform->local_rotation);
    }

    void on_clone(GameObject& target) override
    {
        auto& cloned = target.add_component<Rotator>();
        copy_base_component_data_to(&cloned);
        cloned.rotation_speed = rotation_speed;
        cloned.rotation_axis = rotation_axis;
    }
};
