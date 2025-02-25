#include "RenderingSystem.hpp"

#include "Logger.hpp"
#include "Core/GameObject.hpp"
#include "Core/JobSystem/JobSystem.hpp"

namespace boza
{
    STATIC_VARIABLE_FN(RenderingSystem::thread, {})
    STATIC_VARIABLE_FN(RenderingSystem::stop_flag, { false })

    void RenderingSystem::start()
    {
        thread() = std::thread{ run };
    }

    void RenderingSystem::stop()
    {
        stop_flag() = true;
        if (thread().joinable()) thread().join();
    }


    void RenderingSystem::run()
    {
        std::vector<JobSystem::task_id> tasks(16);

        auto wait_for_tasks = [&tasks]
        {
            for (const auto& task : tasks)
                JobSystem::wait_for_task(task);
            tasks.clear();
        };

        for (const auto* game_object : Scene::get_active_scene().get_game_objects())
        {
            for (auto* behaviour : game_object->behaviours)
                tasks.push_back(JobSystem::push_task([behaviour] { behaviour->start(); }));
        }

        wait_for_tasks();

        while (!stop_flag().load())
        {
            hash_set<GameObject*> game_objects = Scene::get_active_scene().get_game_objects();

            for (const auto* game_object : game_objects)
            {
                for (auto* behaviour : game_object->behaviours)
                    tasks.push_back(JobSystem::push_task([behaviour] { behaviour->update(); }));
            }

            wait_for_tasks();

            for (const auto* game_object : game_objects)
            {
                for (auto* behaviour : game_object->behaviours)
                    tasks.push_back(JobSystem::push_task([behaviour] { behaviour->late_update(); }));
            }

            wait_for_tasks();

            // Logger::trace("Rendering...");
        }
    }
}
