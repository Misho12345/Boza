export module behaviours:oscillator;

import std;
import boza;

using namespace boza;

export class Oscillator final : public Behaviour
{
public:
    glm::vec3 start_pos{ 0.0f };
    glm::vec3 end_pos{ 0.0f };
    float     speed{ 1.0f };
    float     phase{ 0.0f };

    void start() override { start_pos = transform->position; }

    void update(const float dt) override
    {
        phase               += dt * speed;
        const float t       = (std::sinf(phase) + 1.0f) * 0.5f;
        transform->position = mix(start_pos, end_pos, t);
    }
};
