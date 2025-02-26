#include "RenderingSystem.hpp"

#include "Logger.hpp"
#include "Core/GameObject.hpp"
#include "Core/JobSystem/JobSystem.hpp"

namespace boza
{
    void RenderingSystem::on_begin()
    {
        for (const auto* game_object : Scene::get_active_scene().get_game_objects())
        {
            for (auto* behaviour : game_object->behaviours)
                tasks.push_back(JobSystem::push_task([behaviour] { behaviour->start(); }));
        }

        for (const auto& task : tasks)
                JobSystem::wait_for_task(task);
        tasks.clear();
    }


    void RenderingSystem::on_iteration()
    {
        hash_set<GameObject*> game_objects = Scene::get_active_scene().get_game_objects();

        for (const auto* game_object : game_objects)
        {
            for (auto* behaviour : game_object->behaviours) tasks.push_back(JobSystem::push_task([behaviour]
            {
                behaviour->update();
            }));
        }

        for (const auto& task : tasks)
            JobSystem::wait_for_task(task);
        tasks.clear();

        for (const auto* game_object : game_objects)
        {
            for (auto* behaviour : game_object->behaviours) tasks.push_back(JobSystem::push_task([behaviour]
            {
                behaviour->late_update();
            }));
        }

        for (const auto& task : tasks)
            JobSystem::wait_for_task(task);
        tasks.clear();

        // Logger::trace("Rendering...");
    }
}
