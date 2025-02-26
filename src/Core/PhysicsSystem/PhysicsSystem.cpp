#include "PhysicsSystem.hpp"
#include "Core/GameObject.hpp"

namespace boza
{
    void PhysicsSystem::on_iteration()
    {
        for (const auto* object : Scene::get_active_scene().get_game_objects())
        {
            for (auto* behaviour : object->behaviours)
                tasks.push_back(JobSystem::push_task([behaviour] { behaviour->fixed_update(); }));
        }

        for (const auto& task : tasks) JobSystem::wait_for_task(task);
        tasks.clear();
    }
}
