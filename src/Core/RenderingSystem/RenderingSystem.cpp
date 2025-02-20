#include "RenderingSystem.hpp"

#include "Logger.hpp"
#include "Core/GameObject.hpp"

namespace boza
{

    void RenderingSystem::start() { rendering_thread = std::thread(&RenderingSystem::run, this); }
    void RenderingSystem::stop()
    {
        stop_flag = true;
        if (rendering_thread.joinable()) rendering_thread.join();
    }


    void RenderingSystem::run()
    {
        for (const auto* game_object : Scene::get_active_scene().get_game_objects())
        {
            for (auto* behaviour : game_object->behaviours)
                job_system.submit([behaviour] { behaviour->start(); });
        }

        job_system.wait();


        while (!stop_flag)
        {
            std::unordered_set<GameObject*> game_objects = Scene::get_active_scene().get_game_objects();

            for (const auto* game_object : game_objects)
            {
                for (auto* behaviour : game_object->behaviours)
                    job_system.submit([behaviour] { behaviour->update(); });
            }

            job_system.wait();


            for (const auto* game_object : game_objects)
            {
                for (auto* behaviour : game_object->behaviours)
                    job_system.submit([behaviour] { behaviour->late_update(); });
            }

            job_system.wait();

            Logger::trace("Rendering...");
        }
    }
}
