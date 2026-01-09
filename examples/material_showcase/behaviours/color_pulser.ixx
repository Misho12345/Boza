module;

#include <cassert>

export module behaviours:color_pulser;

import std;
import boza;
using namespace boza;

export class RandomColorPulser final : public Behaviour
{
public:
    float speed{ 0.2f };

    void awake() override
    {
        if (game_object->has_component<MeshRenderer>())
            material_ = game_object->get_component<MeshRenderer>().material;
    }

    void update() override
    {
        assert(material_ && "Material not set");

        cooldown_ += Time::delta_time * speed;
        if (cooldown_ > 1.0f)
        {
            cooldown_ = 0.0f;
            color_a_ = color_b_;
            color_b_ = glm::vec3{
                Random::number<float>(),
                Random::number<float>(),
                Random::number<float>()
            };
        }
        const glm::vec3 color = mix(color_a_, color_b_, cooldown_);

        (*material_)["material.albedo_color"] = glm::vec4{ color, 1.0f };

        transform->local_scale = glm::vec3{ glm::sin((cooldown_ + 0.5f) * glm::half_pi<float>()) * 2.0f };
    }

    void on_clone(GameObject& target) override
    {
        auto& cloned = target.add_component<RandomColorPulser>();
        copy_base_component_data_to(&cloned);
        cloned.speed = speed;
        cloned.color_a_ = color_a_;
        cloned.color_b_ = color_b_;
        cloned.cooldown_ = cooldown_;
    }

private:
    glm::vec3 color_a_
    {
        Random::number<float>(),
        Random::number<float>(),
        Random::number<float>()
    };

    glm::vec3 color_b_
    {
        Random::number<float>(),
        Random::number<float>(),
        Random::number<float>()
    };

    Material* material_{ nullptr };
    float cooldown_{ 0.0f };
};
