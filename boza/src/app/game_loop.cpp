module boza.app;

import :game_loop;
import boza.core;
import boza.ecs;
import boza.input;

import std;

namespace boza::app
{
    GameLoop::GameLoop(const GameLoopConfig& config)
        : config_(config)
    {
        const float fixed_timestep = config_.fixed_update_rate > 0 ? 1.0f / config_.fixed_update_rate : 1.0f / 60.0f;
        Time::set_fixed_delta_time(fixed_timestep);
    }

    GameLoop::~GameLoop() = default;

    void GameLoop::start()
    {
        if (running_.load())
        {
            Log::warn("GameLoop is already running");
            return;
        }

        Time::init();
        running_.store(true);

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
        if (!running_.load())
        {
            Log::warn("GameLoop is not running");
            return;
        }

        running_.store(false);

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

        if (active_scene_) active_scene_->on_destroy();

        active_scene_ = scene;

        if (running_.load() && active_scene_)
        {
            active_scene_->on_awake();
            active_scene_->on_start();
        }
    }

    float GameLoop::get_target_fps() const { return config_.target_fps; }
    void  GameLoop::set_target_fps(const float fps) { config_.target_fps = fps; }

    float GameLoop::get_fixed_update_rate() const { return config_.fixed_update_rate; }

    void GameLoop::set_fixed_update_rate(const float rate)
    {
        config_.fixed_update_rate  = rate;
        const float fixed_timestep = rate > 0 ? 1.0f / rate : 1.0f / 60.0f;
        Time::set_fixed_delta_time(fixed_timestep);
    }

    void GameLoop::set_on_render(std::function<void()> func) { on_render_ = std::move(func); }
    void GameLoop::set_poll_events(std::function<void()> func) { poll_events_ = std::move(func); }
    void GameLoop::set_should_close(std::function<bool()> func) { should_close_ = std::move(func); }

    bool GameLoop::is_running() const { return running_; }

    void GameLoop::rendering_loop()
    {
        while (running_.load())
        {
            Time::update();

            {
                std::lock_guard lock(scene_mutex_);

                if (active_scene_)
                {
                    active_scene_->on_start();
                    active_scene_->on_update(Time::delta_time());
                    active_scene_->on_late_update(Time::delta_time());
                }

                if (on_render_) on_render_();
            }

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
        const float fixed_timestep = config_.fixed_update_rate > 0 ? 1.0f / config_.fixed_update_rate : 1.0f / 60.0f;
        float       accumulator    = 0.0f;
        float       last_time      = Time::unscaled_time();

        while (running_.load())
        {
            const float current_time = Time::unscaled_time();
            const float frame_time   = current_time - last_time;
            last_time                = current_time;

            accumulator += frame_time;

            while (accumulator >= fixed_timestep)
            {
                {
                    std::lock_guard lock{ scene_mutex_ };
                    if (active_scene_) active_scene_->on_fixed_update(fixed_timestep);
                }

                accumulator -= fixed_timestep;
            }

            auto sleep_duration = std::chrono::duration<float>(fixed_timestep - accumulator);
            if (sleep_duration.count() > 0) std::this_thread::sleep_for(sleep_duration);
        }
    }

    void GameLoop::wait_for_window_close()
    {
        const float poll_interval = config_.input_poll_rate > 0 ? 1.0f / config_.input_poll_rate : 1.0f / 240.0f;

        while (running_.load())
        {
            if (should_close_ && should_close_())
            {
                stop();
                break;
            }

            if (poll_events_) poll_events_();

            Input::update();

            auto sleep_duration = std::chrono::duration<float>(poll_interval);
            std::this_thread::sleep_for(sleep_duration);
        }
    }
}
