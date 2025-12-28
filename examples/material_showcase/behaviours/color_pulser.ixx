export module behaviours:color_pulser;

import std;
import boza;
using namespace boza;

export class RandomColorPulser final : public Behaviour
{
public:
    float speed{ 0.1f };

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

        auto* material = mesh_renderer_->material;
        if (!material)
        {
            Log::warn("material is nullptr");
            return;
        }

        cooldown_ += dt * speed;
        if (cooldown_ > 1.0f)
        {
            cooldown_ = 0.0f;
            color_a_ = color_b_;
            color_b_ = glm::vec3{
                Random::real<float>(),
                Random::real<float>(),
                Random::real<float>()
            };
        }
        const glm::vec3 color = mix(color_a_, color_b_, cooldown_);

        (*material)["material.albedo_color"] = glm::vec4{ color, 1.0f };
    }

private:
    glm::vec3 color_a_
    {
        Random::real<float>(),
        Random::real<float>(),
        Random::real<float>()
    };

    glm::vec3 color_b_
    {
        Random::real<float>(),
        Random::real<float>(),
        Random::real<float>()
    };

    MeshRenderer* mesh_renderer_{ nullptr };
    float cooldown_{ 0.0f };
};
