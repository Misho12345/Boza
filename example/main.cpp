#include "pch.hpp"
#include "glm/vec3.hpp"

struct transform
{
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};

int main()
{
    boza::Logger::setup();

    const boza::Scene scene{ "default" };

    boza::GameObject obj{ "obj" };

    obj.add_component<transform>(
        glm::vec3{ 0.0f, 0.0f, 0.0f },
        glm::vec3{ 0.0f, 0.0f, 0.0f },
        glm::vec3{ 1.0f, 1.0f, 1.0f });

    auto& [pos, rot, scale] = obj.get_component<transform>();
    boza::Logger::info("Position: {}, {}, {}", pos.x, pos.y, pos.z);
    boza::Logger::info("Rotation: {}, {}, {}", rot.x, rot.y, rot.z);
    boza::Logger::info("Scale: {}, {}, {}", scale.x, scale.y, scale.z);

    for (const auto* object : scene.get_game_objects())
    {
        boza::Logger::info("Object: {}", object->get_data()->name);
    }
}
