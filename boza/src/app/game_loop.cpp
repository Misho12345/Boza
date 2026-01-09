module;

#include <cassert>

module boza.app;

import :game_loop;
import boza.core;
import boza.ecs;
import boza.input;
import std;

namespace boza::app
{
    void GameLoop::init(const GameLoopConfig& config)
    {
        assert(config.rendering_system && "RenderingSystem cannot be null");
        assert(config.window && "Window cannot be null");
        config_ = config;

        Time::fixed_delta_time = 1.0f / (config_.fixed_update_rate > 0 ? config_.fixed_update_rate : 60.0f);
    }

    void GameLoop::start()
    {
        if (running_.load()) return;

        Time::init();
        running_.store(true);

        if (active_scene_)
        {
            active_scene_->evaluate_transforms();
            active_scene_->process_awake_queue();
            active_scene_->evaluate_transforms();
            active_scene_->prepare_initial_frame();
            active_scene_->process_enable_queue();
            active_scene_->process_start_queue();
            active_scene_->evaluate_transforms();
            active_scene_->mark_started();
        }

        physics_thread_   = std::thread([this] { physics_loop(); });
        rendering_thread_ = std::thread([this] { rendering_loop(); });
    }

    void GameLoop::stop()
    {
        if (!running_.load()) return;

        running_.store(false);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);

        if (rendering_thread_.joinable()) rendering_thread_.join();
        if (physics_thread_.joinable()) physics_thread_.join();

        if (active_scene_)
        {
            std::lock_guard lock{ scene_mutex_ };
            active_scene_->on_destroy();
        }
    }

    Scene* GameLoop::get_active_scene()
    {
        std::lock_guard lock{ scene_mutex_ };
        return active_scene_;
    }

    void GameLoop::set_active_scene(Scene* scene)
    {
        std::lock_guard lock{ scene_mutex_ };
        if (active_scene_) active_scene_->on_destroy();
        active_scene_ = scene;
    }

    float GameLoop::get_target_fps() const { return config_.target_fps; }
    void  GameLoop::set_target_fps(const float fps) { config_.target_fps = fps; }

    bool GameLoop::is_running() const { return running_; }

    void GameLoop::update_scene(Scene* scene)
    {
        if (!scene) return;

        scene->flush_structural_changes();
        scene->process_awake_queue();
        scene->process_enable_queue();
        scene->process_start_queue();
        scene->rebuild_update_caches();
        scene->on_update();
        scene->flush_structural_changes();
        scene->on_late_update();
    }

    void GameLoop::rendering_loop()
    {
        while (running_.load())
        {
            Time::update();

            {
                std::lock_guard lock{ scene_mutex_ };

                if (auto* p = Scene::persistent_scene()) update_scene(p);
                if (active_scene_) update_scene(active_scene_);

                Input::flush_input_queue();
                config_.rendering_system->run();
            }

            if (!config_.vsync && config_.target_fps > 0)
            {
                const float frame_time = 1.0f / config_.target_fps;
                const float elapsed    = Time::unscaled_delta_time_;
                if (elapsed < frame_time)
                    std::this_thread::sleep_for(std::chrono::duration<float>(frame_time - elapsed));
            }
        }
    }

    void GameLoop::physics_loop()
    {
        const float fixed_timestep = config_.fixed_update_rate > 0 ? 1.0f / config_.fixed_update_rate : 1.0f / 60.0f;
        float accumulator    = 0.0f;
        float last_time      = Time::unscaled_time_;

        while (running_.load())
        {
            const float current_time = Time::unscaled_time_;
            const float frame_time   = current_time - last_time;
            last_time          = current_time;
            accumulator       += frame_time;

            while (accumulator >= fixed_timestep)
            {
                {
                    std::lock_guard lock{ scene_mutex_ };
                    if (const auto* p = Scene::persistent_scene()) p->on_fixed_update();
                    if (active_scene_) active_scene_->on_fixed_update();
                }

                accumulator -= fixed_timestep;
            }

            if (float sleep = fixed_timestep - accumulator; sleep > 0)
                std::this_thread::sleep_for(std::chrono::duration<float>(sleep));
        }
    }

    void GameLoop::wait_for_window_close()
    {
        const float poll_interval = config_.input_poll_rate > 0 ? 1.0f / config_.input_poll_rate : 1.0f / 240.0f;

        while (running_.load())
        {
            if (config_.window->should_close())
            {
                stop();
                break;
            }

            config_.window->poll_events();
            config_.window->apply_cursor_state_if_needed();
            Input::update();

            std::this_thread::sleep_for(std::chrono::duration<float>(poll_interval));
        }
    }
}
