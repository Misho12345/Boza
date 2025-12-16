module game:color_pulser;

import std;
import boza;
using namespace boza;

namespace game
{
    class ColorPulser final : public Behaviour
    {
    public:
        Material* material{ nullptr };
        glm::vec4 color_a{ 1.0f, 0.0f, 0.0f, 1.0f };
        glm::vec4 color_b{ 0.0f, 0.0f, 1.0f, 1.0f };
        float speed{ 1.0f };
        float phase{ 0.0f };

        void update(const float dt) override
        {
            if (!material) return;

            phase += dt * speed;
            const float t = (std::sinf(phase) + 1.0f) * 0.5f;
            const glm::vec4 color = glm::mix(color_a, color_b, t);
            (*material)["material.albedo_color"] = color;
        }
    };
}