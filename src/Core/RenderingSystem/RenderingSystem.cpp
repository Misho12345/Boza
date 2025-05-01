#include "RenderingSystem.hpp"

#include "Logger.hpp"
#include "Core/GameObject.hpp"

#include "Vulkan/Device.hpp"
#include "Vulkan/Renderer.hpp"

namespace boza
{
    void RenderingSystem::on_begin()
    {
        if (!Renderer::initialize())
        {
            Logger::critical("Failed to initialize renderer");
            stop();
            return;
        }

        for (const auto* game_object : Scene::get_active_scene().get_game_objects())
        {
            for (auto* behaviour : game_object->behaviours)
                behaviour->start();
        }
    }


    void RenderingSystem::on_iteration()
    {
        hash_set<GameObject*> game_objects = Scene::get_active_scene().get_game_objects();

        for (const auto* game_object : game_objects)
        {
            for (auto* behaviour : game_object->behaviours)
                behaviour->update();
        }

        for (const auto* game_object : game_objects)
        {
            for (auto* behaviour : game_object->behaviours)
                behaviour->late_update();
        }

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
