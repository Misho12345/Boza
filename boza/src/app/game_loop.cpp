module boza.app;

import std;

import :game_loop;

import boza.ecs;
import boza.core;
import boza.app.game_settings;
import boza.rhi.render_context;

namespace boza::app
{
    void GameLoop::init(const GameLoopConfig& config)
    {
        config_ = config;
        Time::fixed_delta_time = config_.physics_update_rate > 0.0f
            ? 1.0f / config_.physics_update_rate
            : 0.0f;

        auto& registry = SystemRegistry::instance();
        registry.initialize_all(Scene::world(), config_.physics_update_rate);
    }

    bool GameLoop::run_engine_begin_stages() const
    {
        SystemRegistry::instance().call_engine_begin_stages();

        if (rhi::RenderContext::initialized()) return true;

        Log::critical("Engine begin stages failed to initialize the render context");
        return false;
    }

    void GameLoop::run() const
    {
        const auto& registry = SystemRegistry::instance();
        registry.call_startup_stages();

        const auto& world = Scene::world();
        world.set_target_fps(config_.target_fps > 0.0f ? config_.target_fps : 0.0f);
        world.reset_clock();

        Time::init();

        while (!world.should_quit())
        {
            Time::update();
            const bool keep_running = world.progress(Time::delta_time());
            Scene::remove_objects_for_destruction();

            if (!keep_running) break;
        }

        registry.call_destroy_stages();
    }

    float GameLoop::get_target_fps() const
    {
        return config_.target_fps;
    }

    void GameLoop::set_target_fps(const float fps)
    {
        config_.target_fps = fps;
        const auto& world = Scene::world();
        world.set_target_fps(config_.target_fps > 0.0f ? config_.target_fps : 0.0f);
        world.reset_clock();
    }
}
