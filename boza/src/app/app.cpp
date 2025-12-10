module boza.app;

import boza.platform;
import boza.core;
import boza.gfx;
import boza.input;

import boza.detail;
import boza.rhi.api;

import :game_loop;
import :rendering_system;
import :game_settings;

namespace boza
{
    using app::GameLoop;
    using app::GameLoopConfig;
    using app::GameSettings;
    using app::RenderingSystem;

    using platform::Window;

    using detail::AudioApi;
    using detail::AssetPaths;

    struct App::Impl
    {
        std::unique_ptr<Window>          window;
        std::unique_ptr<RenderingSystem> rendering_system;
        std::unique_ptr<GameLoop>        game_loop;
        std::shared_ptr<Scene>           active_scene;
        bool initialized{ false };
    };

    App::App() : impl_(std::make_unique<Impl>()) { Log::init(); }
    App::~App() = default;

    bool App::init()
    {
        if (!GameSettings::load_from_file(AssetPaths::asset("game_settings.json").string()))
        {
            Log::warn("Could not load game settings from file, using defaults");
        }

        impl_->window = std::make_unique<Window>(
            GameSettings::window.width,
            GameSettings::window.height,
            GameSettings::window.title,
            GameSettings::window.fullscreen
        );

        if (!Window::init())
        {
            Log::critical("Failed to initialize windowing system");
            return false;
        }

        on_setup_scene();

        if (!impl_->active_scene) impl_->active_scene = create_scene("Default Scene");

        impl_->rendering_system = std::make_unique<RenderingSystem>();
        if (!impl_->rendering_system->init(*impl_->window, impl_->active_scene))
        {
            Log::error("Failed to initialize rendering system");
            return false;
        }

        on_graphics_ready();

        GameLoopConfig loop_config
        {
            .target_fps = GameSettings::graphics.target_fps,
            .fixed_update_rate = GameSettings::physics.fixed_update_rate,
            .input_poll_rate = GameSettings::input.poll_rate,
            .vsync = GameSettings::graphics.vsync
        };

        impl_->game_loop = std::make_unique<GameLoop>(loop_config);

        if (impl_->active_scene) { impl_->game_loop->set_active_scene(impl_->active_scene); }

        impl_->game_loop->set_on_render([this] { impl_->rendering_system->run(); });
        impl_->game_loop->set_poll_events([this] { impl_->window->poll_events(); });
        impl_->game_loop->set_should_close([this] { return impl_->window->should_close(); });
        impl_->game_loop->set_apply_cursor_state([this] { impl_->window->apply_cursor_state_if_needed(); });

        Input::init(impl_->window->native_handle());

        impl_->initialized = true;
        return true;
    }

    void App::run()
    {
        if (!impl_->game_loop || !impl_->initialized)
        {
            Log::critical("App::run() called before App::init(). Please call init() first.");
            return;
        }

        Log::trace("Starting application...");

        impl_->game_loop->start();
        impl_->game_loop->wait_for_window_close();

        Log::trace("Application stopping");
        shutdown();
        Log::trace("Application stopped");
    }

    void App::shutdown()
    {
        if (!impl_->initialized) return;

        if (impl_->rendering_system) impl_->rendering_system->wait_idle();
        on_shutdown();
        if (impl_->rendering_system) impl_->rendering_system->destroy();

        Input::shutdown();

        if (impl_->window) impl_->window->destroy();
        Window::terminate();

        impl_->initialized = false;
        Log::trace("App shutdown complete");
    }

    void App::toggle_fullscreen() const
    {
        if (impl_->window) impl_->window->toggle_fullscreen();
    }

    void App::set_cursor_state(const CursorState state) const
    {
        if (!impl_->window) return;

        const platform::CursorState plat_state = [&state] -> platform::CursorState
        {
            switch (state)
            {
                case CursorState::Normal: return platform::CursorState::Normal;
                case CursorState::Hidden: return platform::CursorState::Hidden;
                case CursorState::Locked: return platform::CursorState::Locked;
                case CursorState::HiddenLocked: return platform::CursorState::HiddenLocked;
            }

            std::unreachable();
        }();

        impl_->window->set_cursor_state(plat_state);
    }

    void App::set_active_scene(const std::shared_ptr<Scene>& scene)
    {
        impl_->active_scene = scene;
        if (impl_->game_loop) impl_->game_loop->set_active_scene(scene);
    }

    std::shared_ptr<Scene> App::get_active_scene() const { return impl_->active_scene; }

    std::shared_ptr<Scene> App::create_scene(const std::string& name)
    {
        auto scene = std::make_shared<Scene>(name);
        return scene;
    }

    void App::set_target_fps(const float fps)
    {
        if (impl_->game_loop) impl_->game_loop->set_target_fps(fps);
    }

    void App::set_fixed_update_rate(const float rate)
    {
        if (impl_->game_loop) impl_->game_loop->set_fixed_update_rate(rate);
    }

    void App::register_custom_material(const std::string& name, Material* material) const
    {
        if (impl_->rendering_system)
        {
            impl_->rendering_system->material_loader().register_material(name, material);
        }
    }
}
