#include "RenderingSystem.hpp"

#include "Logger.hpp"
#include "Core/GameObject.hpp"
#include "Core/JobSystem/JobSystem.hpp"
#include "Vulkan/CommandPool.hpp"

#include "Vulkan/Instance.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Swapchain.hpp"
#include "Vulkan/Pipeline.hpp"

namespace boza
{
    static bool try_(const bool res, const std::string_view& message)
    {
        if (!res)
        {
            Logger::error(message);
            RenderingSystem::stop();
        }

        return res;
    }

    void RenderingSystem::on_begin()
    {
        tasks.reserve(16);

        if (!try_(Instance::create("Boza app"), "Failed to create vulkan instance!")) return;
        if (!try_(Device::create(), "Failed to create logical device!")) return;
        if (!try_(CommandPool::create(), "Failed to create command pool!")) return;
        if (!try_(Swapchain::create(), "Failed to create swapchain!")) return;
        if (!try_(Pipeline::create(), "Failed to create graphics pipeline!")) return;

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

        if (!Swapchain::render())
        {
            Logger::error("Failed to render frame!");
            stop();
        }
    }

    void RenderingSystem::on_end()
    {
        Device::wait_idle();

        Pipeline::destroy();
        Swapchain::destroy();
        CommandPool::destroy();
        Device::destroy();
        Instance::destroy();
    }
}
