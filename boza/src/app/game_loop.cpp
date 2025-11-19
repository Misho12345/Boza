module boza.app;

import :game_loop;
import boza.core;
import boza.ecs;

import std;

namespace boza::app
{
    GameLoop::GameLoop(const GameLoopConfig& config)
        : config_(config)
    {
        Time::set_fixed_delta_time(config_.fixed_timestep);
    }

    GameLoop::~GameLoop() { stop(); }

    void GameLoop::start()
    {
        if (running_)
        {
            Log::warn("GameLoop is already running");
            return;
        }

        Time::init();
        running_         = true;
        physics_running_ = true;

        if (active_scene_)
        {
            active_scene_->on_awake();
            active_scene_->on_start();
        }

        physics_thread_   = std::thread([this] { physics_loop(); });
        rendering_thread_ = std::thread([this] { rendering_loop(); });
    }

    void GameLoop::stop()
    {
        if (!running_.load() && !physics_running_.load()) { return; }

        running_.store(false);
        physics_running_.store(false);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);

        if (rendering_thread_.joinable()) rendering_thread_.join();
        if (physics_thread_.joinable()) physics_thread_.join();

        if (active_scene_)
        {
            std::lock_guard lock{ scene_mutex_ };
            if (active_scene_)
            {
                active_scene_->on_destroy();
                active_scene_.reset();
            }
        }
    }

    std::shared_ptr<Scene> GameLoop::get_active_scene()
    {
        std::lock_guard lock{ scene_mutex_ };
        return active_scene_;
    }

    void GameLoop::set_active_scene(const std::shared_ptr<Scene>& scene)
    {
        std::lock_guard lock{ scene_mutex_ };

        if (active_scene_) { active_scene_->on_destroy(); }

        active_scene_ = scene;

        if (running_)
        {
            if (active_scene_)
            {
                active_scene_->on_awake();
                active_scene_->on_start();
            }
        }
    }

    float GameLoop::get_target_fps() const { return config_.target_fps; }
    void GameLoop::set_target_fps(float fps) { config_.target_fps = fps; }

    float GameLoop::get_fixed_timestep() const { return config_.fixed_timestep; }
    void GameLoop::set_fixed_timestep(float timestep)
    {
        config_.fixed_timestep = timestep;
        Time::set_fixed_delta_time(config_.fixed_timestep);
    }

    void GameLoop::set_on_render(std::function<void()> func) { on_render_ = std::move(func); }
    void GameLoop::set_should_close(std::function<bool()> func) { should_close_ = std::move(func); }

    bool GameLoop::is_running() const { return running_; }

    void GameLoop::rendering_loop()
    {
        while (running_)
        {
            Time::update();

            if (should_close_ && should_close_())
            {
                running_.store(false);
                break;
            }

            {
                std::lock_guard lock(scene_mutex_);

                if (active_scene_)
                {
                    active_scene_->on_start(); // For newly created objects
                    active_scene_->on_update(Time::delta_time());
                    active_scene_->on_late_update(Time::delta_time());
                }
            }

            if (on_render_) on_render_();

            if (config_.target_fps > 0)
            {
                const float frame_time = 1.0f / config_.target_fps;
                const float elapsed    = Time::unscaled_delta_time();

                if (elapsed < frame_time)
                {
                    auto sleep_duration = std::chrono::duration<float>(frame_time - elapsed);
                    std::this_thread::sleep_for(sleep_duration);
                }
            }
        }
    }

    void GameLoop::physics_loop()
    {
        float accumulator = 0.0f;
        float last_time   = Time::unscaled_time();

        while (physics_running_)
        {
            const float current_time = Time::unscaled_time();
            const float frame_time   = current_time - last_time;
            last_time          = current_time;

            accumulator += frame_time;

            while (accumulator >= config_.fixed_timestep)
            {
                {
                    std::lock_guard lock{ scene_mutex_ };
                    if (active_scene_) active_scene_->on_fixed_update(config_.fixed_timestep);
                }

                accumulator -= config_.fixed_timestep;
            }

            auto sleep_duration = std::chrono::duration<float>(config_.fixed_timestep - accumulator);
            if (sleep_duration.count() > 0) std::this_thread::sleep_for(sleep_duration);
        }
    }
}

