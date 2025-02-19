#include "pch.hpp"
#include "Core/Components/Transform.hpp"
#include "glm/vec3.hpp"


int main()
{
    boza::Logger::setup();

    const boza::Scene scene{ "default" };

    boza::GameObject obj{ "obj" };
    const boza::GameObject obj2{ "obj2" };

    auto& transform = obj.get_transform();
    boza::Logger::info("Obj: {} ({})", obj.get_name(), (uint32_t)obj.get_id());
    boza::Logger::info("Position: {}, {}, {}", transform.position.x, transform.position.y, transform.position.z);
    boza::Logger::info("Rotation: {}, {}, {}", transform.rotation.x, transform.rotation.y, transform.rotation.z);
    boza::Logger::info("Scale: {}, {}, {}", transform.scale.x, transform.scale.y, transform.scale.z);

    for (const auto* object : scene.get_game_objects())
    {
        boza::Logger::trace("---------");
        boza::Logger::info("Object: {} ({})", object->get_name(), (uint32_t)object->get_id());
        const glm::vec3& pos = object->get_transform().position;
        boza::Logger::info("Position: {}, {}, {}", pos.x, pos.y, pos.z);
    }

    obj.add_component<struct test : boza::Component {}>();
}
