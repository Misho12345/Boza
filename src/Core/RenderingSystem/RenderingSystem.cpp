#include "RenderingSystem.hpp"

#include "Logger.hpp"
#include "Core/GameObject.hpp"

#include "GPU/Vulkan/Core/Device.hpp"
#include "Render/Renderer.hpp"

namespace boza
{
    void RenderingSystem::on_begin()
    {
        if (!Renderer::initialize())
        {
            Logger::critical("Failed to initialize renderer");
            stop_flag.store(true);
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
        const duration dt = get_delta_time();

        for (const auto* game_object : game_objects)
        {
            for (auto* behaviour : game_object->behaviours)
                behaviour->update(dt);
        }

        for (const auto* game_object : game_objects)
        {
            for (auto* behaviour : game_object->behaviours)
                behaviour->late_update(dt);
        }

        if (!Renderer::render())
        {
            Logger::error("Failed to render frame!");
            stop();
        }
    }

    void RenderingSystem::on_end()
    {
        Logger::trace("RenderingSystem::on_end()");
        Device::wait_idle();
        Renderer::shutdown();
    }
}
