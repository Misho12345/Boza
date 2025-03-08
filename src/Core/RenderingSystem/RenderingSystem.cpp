#include "RenderingSystem.hpp"

#include "Logger.hpp"
#include "Core/GameObject.hpp"
#include "Core/JobSystem/JobSystem.hpp"

#include "Vulkan/Device.hpp"
#include "Vulkan/Swapchain.hpp"
#include "Vulkan/Renderer.hpp"

namespace boza
{
    void RenderingSystem::on_begin()
    {
        tasks.reserve(16);

        if (!Renderer::initialize())
        {
            Logger::critical("Failed to initialize renderer");
            stop();
            return;
        }

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

        if (!Renderer::render())
        {
            Logger::error("Failed to render frame!");
            stop();
        }
    }

    void RenderingSystem::on_end()
    {
        Device::wait_idle();
        Renderer::shutdown();
    }
}
