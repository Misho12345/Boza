module;

#include <cassert>

module boza.app;

import boza.platform;
import boza.core;
import boza.gfx;
import boza.input;

import boza.detail;
import boza.rhi.api;
import boza.gfx.rendering_system;
import boza.gfx.material_loader;

import :game_loop;

namespace boza
{
    using app::GameLoop;
    using app::GameLoopConfig;

    using gfx::RenderingSystem;

    using platform::Window;

    using detail::AssetPaths;
    using detail::GameSettings;

    struct App::Impl
    {
        Window          window{};
        RenderingSystem rendering_system{};
        GameLoop        game_loop{};

        node_map<std::string, Scene> scenes{};
        Scene* active_scene{ nullptr };
        Camera* primary_camera{ nullptr };

        bool initialized{ false };
    };

    App::App() : impl_(std::make_unique<Impl>())
    {
        assert(s_instance_ == nullptr && "App instance already exists");
        Log::init();
        s_instance_ = this;
    }

    App::~App() { s_instance_ = nullptr; }

    bool App::init()
    {
        if (!GameSettings::load_from_file(AssetPaths::asset("game_settings.json").string()))
        {
            Log::warn("Could not load game settings from file, using defaults");
        }

        if (!Window::init())
        {
            Log::critical("Failed to initialize windowing system");
            return false;
        }

        impl_->window.init(
            GameSettings::window.width,
            GameSettings::window.height,
            GameSettings::window.title,
            GameSettings::window.fullscreen
        );

        if (!impl_->rendering_system.init(impl_->window))
        {
            Log::error("Failed to initialize rendering system");
            return false;
        }

        impl_->game_loop.init({
            .rendering_system = &impl_->rendering_system,
            .window = &impl_->window,
            .target_fps = GameSettings::graphics.target_fps,
            .fixed_update_rate = GameSettings::physics.fixed_update_rate,
            .input_poll_rate = GameSettings::input.poll_rate,
            .vsync = GameSettings::graphics.vsync
        });

        Input::init(&impl_->window);

        setup();

        if (!impl_->active_scene) set_active_scene(create_scene("Default Scene"));

        post_setup();

        impl_->initialized = true;
        return true;
    }

    void App::run()
    {
        if (!impl_->initialized)
        {
            Log::critical("App::run() called before App::init(). Please call init() first.");
            return;
        }

        Log::trace("Starting application...");

        impl_->game_loop.start();
        impl_->game_loop.wait_for_window_close();

        shutdown();
    }

    void App::shutdown()
    {
        if (!impl_->initialized) return;

        impl_->rendering_system.wait_idle();
        on_shutdown();
        impl_->rendering_system.destroy();

        Input::shutdown();

        impl_->window.destroy();
        Window::terminate();

        impl_->initialized = false;
        Log::trace("App shutdown complete");
    }

    void App::toggle_fullscreen()
    {
        if (s_instance_) s_instance_->impl_->window.toggle_fullscreen();
    }

    void App::set_cursor_state(const CursorState state)
    {
        if (s_instance_) s_instance_->impl_->window.set_cursor_state(state);
    }

    CursorState App::get_cursor_state()
    {
        return s_instance_ ? s_instance_->impl_->window.cursor_state() : CursorState::Normal;
    }

    void App::set_active_scene(Scene& scene)
    {
        if (!s_instance_) return;

        s_instance_->impl_->active_scene = &scene;
        s_instance_->impl_->game_loop.set_active_scene(s_instance_->impl_->active_scene);
        s_instance_->impl_->rendering_system.set_active_scene(s_instance_->impl_->active_scene);
    }

    Scene* App::get_active_scene() { return s_instance_ ? s_instance_->impl_->active_scene : nullptr; }

    void App::set_primary_camera(Camera& camera)
    {
        if (!s_instance_) return;

        s_instance_->impl_->primary_camera = &camera;
        s_instance_->impl_->rendering_system.set_primary_camera(&camera);
    }

    Camera* App::get_primary_camera()
    {
        return s_instance_ ? s_instance_->impl_->primary_camera : nullptr;
    }

    Scene& App::create_scene(const std::string_view name)
    {
        assert(s_instance_ && "App instance not created");

        auto [it, inserted] = s_instance_->impl_->scenes.emplace(name, Scene(name));
        return it->second;
    }

    Scene* App::get_scene(const std::string_view name)
    {
        if (!s_instance_) return nullptr;

        auto it = s_instance_->impl_->scenes.find(name);
        return it != s_instance_->impl_->scenes.end() ? &it->second : nullptr;
    }

    void App::set_target_fps(const float fps)
    {
        if (s_instance_ ) s_instance_->impl_->game_loop.set_target_fps(fps);
    }

    float App::get_target_fps()
    {
        return s_instance_ ? s_instance_->impl_->game_loop.get_target_fps() : 0.0f;
    }

    void App::set_fixed_update_rate(const float rate)
    {
        if (s_instance_) s_instance_->impl_->game_loop.set_fixed_update_rate(rate);
    }

    float App::get_fixed_update_rate()
    {
        return s_instance_
                   ? s_instance_->impl_->game_loop.get_fixed_update_rate()
                   : 0.0f;
    }
}
