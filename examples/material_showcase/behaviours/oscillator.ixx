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

    void update(const float dt) override
    {
        phase += dt * speed;

        const float s = glm::sin(phase);
        const glm::vec3 offset = normalize(axis) * (s * amplitude);
        transform->local_position = base_local_ + offset;
    }

private:
    glm::vec3 base_local_{};
};
