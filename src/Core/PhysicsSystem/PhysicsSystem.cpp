#include "PhysicsSystem.hpp"
#include "Core/GameObject.hpp"

namespace boza
{
    void PhysicsSystem::on_iteration()
    {
        for (const auto* object : Scene::get_active_scene().get_game_objects())
        {
            for (auto* behaviour : object->behaviours)
                behaviour->fixed_update();
        }
    }
}
