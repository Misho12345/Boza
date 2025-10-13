#include "boza/core/GameLoop.hpp"
#include "boza/core/Time.hpp"
#include "boza/core/Logger.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

namespace boza
{
    struct GameLoop::GameLoopData
    {
        std::atomic<bool> running_{ false };
        std::atomic<bool> physics_running_{ false };

        std::shared_ptr<Scene> active_scene_;

        std::thread rendering_thread_;
        std::thread physics_thread_;

        std::mutex scene_mutex_;

        std::function<void()> on_render;
        std::function<bool()> should_close;
    };

    GameLoop::GameLoop(const GameLoopConfig& config)
        : config_(config),
          data_(std::make_unique<GameLoopData>())
    {
        Time::set_fixed_delta_time(config_.fixed_timestep);
    }

    GameLoop::~GameLoop() { stop(); }


    void GameLoop::start() const
    {
        if (data_->running_)
        {
            Logger::warn("GameLoop is already running");
            return;
        }

        Time::init();
        data_->running_         = true;
        data_->physics_running_ = true;

        if (data_->active_scene_)
        {
            data_->active_scene_->on_awake();
            data_->active_scene_->on_start();
        }

        data_->physics_thread_   = std::thread([this] { physics_loop(); });
        data_->rendering_thread_ = std::thread([this] { rendering_loop(); });

        // Logger::trace("GameLoop started");
    }

    void GameLoop::stop() const
    {
        if (!data_->running_.load() && !data_->physics_running_.load()) { return; }

        // Logger::trace("Stopping GameLoop...");

        data_->running_.store(false);
        data_->physics_running_.store(false);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);

        if (data_->rendering_thread_.joinable()) data_->rendering_thread_.join();
        if (data_->physics_thread_.joinable()) data_->physics_thread_.join();

        if (data_->active_scene_)
        {
            std::lock_guard lock{ data_->scene_mutex_ };
            if (data_->active_scene_)
            {
                data_->active_scene_->on_destroy();
                data_->active_scene_.reset();
            }
        }

        // Logger::trace("GameLoop stopped");
    }


    void GameLoop::set_active_scene(const std::shared_ptr<Scene>& scene) const
    {
        std::lock_guard lock{ data_->scene_mutex_ };

        if (data_->active_scene_) { data_->active_scene_->on_destroy(); }

        data_->active_scene_ = scene;

        if (data_->running_)
        {
            if (data_->active_scene_)
            {
                data_->active_scene_->on_awake();
                data_->active_scene_->on_start();
            }
        }
    }

    std::shared_ptr<Scene> GameLoop::get_active_scene() const
    {
        std::lock_guard lock{ data_->scene_mutex_ };
        return data_->active_scene_;
    }

    void GameLoop::set_on_render(std::function<void()> func) const { data_->on_render = std::move(func); }
    void GameLoop::set_should_close(std::function<bool()> func) const { data_->should_close = std::move(func); }

    bool GameLoop::is_running() const { return data_->running_; }


    void GameLoop::rendering_loop() const
    {
        // Logger::trace("Rendering loop started");

        while (data_->running_)
        {
            Time::update();

            if (data_->should_close && data_->should_close())
            {
                data_->running_.store(false);
                break;
            }

            {
                std::lock_guard lock(data_->scene_mutex_);

                if (data_->active_scene_)
                {
                    data_->active_scene_->on_start(); // For newly created objects
                    data_->active_scene_->on_update(Time::delta_time());
                    data_->active_scene_->on_late_update(Time::delta_time());
                }
            }

            if (data_->on_render) data_->on_render();

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

        // Logger::trace("Rendering loop finished");
    }

    void GameLoop::physics_loop() const
    {
        // Logger::trace("Physics loop started");

        float accumulator = 0.0f;
        float last_time   = Time::unscaled_time();

        while (data_->physics_running_)
        {
            const float current_time = Time::unscaled_time();
            const float frame_time   = current_time - last_time;
            last_time          = current_time;

            accumulator += frame_time;

            while (accumulator >= config_.fixed_timestep)
            {
                {
                    std::lock_guard lock{ data_->scene_mutex_ };
                    if (data_->active_scene_) data_->active_scene_->on_fixed_update(config_.fixed_timestep);
                }

                accumulator -= config_.fixed_timestep;
            }

            auto sleep_duration = std::chrono::duration<float>(config_.fixed_timestep - accumulator);
            if (sleep_duration.count() > 0) std::this_thread::sleep_for(sleep_duration);
        }

        // Logger::trace("Physics loop finished");
    }
}
