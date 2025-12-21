export module behaviours:color_pulser;

import std;
import boza;
using namespace boza;

export class ColorPulser final : public Behaviour
{
public:
    glm::vec4 color_a{ 1.0f, 0.0f, 0.0f, 1.0f };
    glm::vec4 color_b{ 0.0f, 0.0f, 1.0f, 1.0f };
    float     speed{ 1.0f };

    void awake() override
    {
        mesh_renderer_ = game_object->try_get_component<MeshRenderer>();
    }

    void update(const float dt) override
    {
        if (!mesh_renderer_)
        {
            mesh_renderer_ = game_object->try_get_component<MeshRenderer>();
            if (!mesh_renderer_) return;
        }

        auto* material = mesh_renderer_->material();
        if (!material)
        {
            Log::warn("material is nullptr");
            return;
        }

        phase_ += dt * speed;
        const float     t     = (std::sinf(phase_) + 1.0f) * 0.5f;
        const glm::vec4 color = glm::mix(color_a, color_b, t);

        (*material)["material.albedo_color"] = color;
    }

private:
    MeshRenderer* mesh_renderer_{ nullptr };
    float phase_{ 0.0f };
};
