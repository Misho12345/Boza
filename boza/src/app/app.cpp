module boza.app;

import boza.app.game_settings;

import boza.core;
import boza.ecs;
import boza.gfx;
import boza.input;
import boza.platform;

import boza.detail;

import boza.rhi.render_context;

import :game_loop;

namespace boza
{
    using platform::Window;
    using detail::AssetPaths;

    struct App::Impl
    {
        Window        window{};
        app::GameLoop game_loop{};
        bool          initialized{ false };
    };


    App::App() : impl_{ std::make_unique<Impl>() }
    {
        assert(s_instance_ == nullptr, "App instance already exists");
        Log::init();
        s_instance_ = this;
    }

    App::~App() { s_instance_ = nullptr; }


    bool App::init()
    {
        if (!app::GameSettings::load_from_file(AssetPaths::asset("game_settings.json")))
        {
            Log::warn("Could not load game settings from file, using defaults");
        }

        if (!Window::init())
        {
            Log::critical("Failed to initialize windowing system");
            return false;
        }

        impl_->window.init(
            app::GameSettings::window.width,
            app::GameSettings::window.height,
            app::GameSettings::window.title,
            app::GameSettings::window.fullscreen
        );

        rhi::RenderContext::set_window(&impl_->window);

        impl_->game_loop.init({
            .target_fps       = app::GameSettings::gameplay.target_fps,
            .physics_update_rate = app::GameSettings::gameplay.physics_update_rate
        });

        if (!impl_->game_loop.run_engine_begin_stages())
        {
            Log::critical("Failed to initialize engine begin stages");
            return false;
        }

        setup();

        impl_->initialized = true;
        return true;
    }

    void App::run() const
    {
        if (!impl_->initialized)
        {
            Log::critical("App::run() called before App::init(). Please call init() first.");
            return;
        }

        impl_->game_loop.run();

        shutdown();
    }

    void App::shutdown() const
    {
        if (!impl_->initialized) return;

        impl_->window.destroy();
        Window::terminate();

        impl_->initialized = false;
        Log::trace("App shutdown complete");
    }


    void App::toggle_fullscreen()
    {
        assert(s_instance_ != nullptr, "App instance does not exist");
        s_instance_->impl_->window.toggle_fullscreen();
    }

    void App::quit() { Scene::world().quit(); }


    void App::set_cursor_state(const CursorState state)
    {
        assert(s_instance_ != nullptr, "App instance does not exist");
        s_instance_->impl_->window.set_cursor_state(state);
    }

    CursorState App::get_cursor_state()
    {
        assert(s_instance_ != nullptr, "App instance does not exist");
        return s_instance_->impl_->window.cursor_state();
    }


    void App::set_target_fps(const float fps)
    {
        assert(s_instance_ != nullptr, "App instance does not exist");
        s_instance_->impl_->game_loop.set_target_fps(fps);
    }

    float App::get_target_fps()
    {
        assert(s_instance_ != nullptr, "App instance does not exist");
        return s_instance_->impl_->game_loop.get_target_fps();
    }
}
