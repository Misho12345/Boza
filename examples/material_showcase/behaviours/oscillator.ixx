export module behaviours:oscillator;

import std;
import boza;

using namespace boza;

export class Oscillator final : public Behaviour
{
public:
    float speed{ 1.0f };
    float amplitude{ 1.0f };
    float phase{ 0.0f };
    glm::vec3 axis{ 0.0f, 1.0f, 0.0f };

    void start() override
    {
        base_local_ = transform->local_position;
    }

    void update() override
    {
        phase += Time::delta_time() * speed;

        const float s = glm::sin(phase);
        const glm::vec3 offset = normalize(axis) * (s * amplitude);
        transform->local_position = base_local_ + offset;
    }

    void on_clone(GameObject& target) override
    {
        auto& cloned = target.add_component<Oscillator>();
        copy_base_component_data_to(&cloned);
        cloned.speed = speed;
        cloned.amplitude = amplitude;
        cloned.phase = phase;
        cloned.axis = axis;
        cloned.base_local_ = base_local_;
    }

private:
    glm::vec3 base_local_{};
};
