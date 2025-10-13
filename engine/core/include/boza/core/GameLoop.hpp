#pragma once
#include "Property.hpp"
#include "Time.hpp"
#include "boza/pch.hpp"
#include "boza/API.hpp"
#include "boza/core/Scene.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

namespace boza
{
    struct GameLoopConfig
    {
        float target_fps{ 0.0f };
        float fixed_timestep{ 1.0f / 60.0f };
        bool  vsync{ true };
    };

    class BOZA_API GameLoop
    {
    public:
        explicit GameLoop(const GameLoopConfig& config = {});
        ~GameLoop();

        void start() const;
        void stop() const;

        PropertyGet<bool> running{ GET { return is_running(); } };

        PropertyGetSet<std::shared_ptr<Scene>> active_scene
        {
            GET { return get_active_scene(); },
            SET(scene) { set_active_scene(scene); }
        };

        PropertyGetSet<float> target_fps
        {
            GET { return config_.target_fps; },
            SET(fps) { config_.target_fps = fps; }
        };

        PropertyGetSet<float> fixed_timestep
        {
            GET { return config_.fixed_timestep; },
            SET(timestep)
            {
                config_.fixed_timestep = timestep;
                Time::set_fixed_delta_time(config_.fixed_timestep);
            }
        };

        PropertySet<std::function<void()>> on_render{ SET(func) { set_on_render(func); } };
        PropertySet<std::function<bool()>> should_close{ SET(func) { set_should_close(func); } };

    private:
        std::shared_ptr<Scene> get_active_scene() const;
        void                   set_active_scene(const std::shared_ptr<Scene>& scene) const;

        void set_on_render(std::function<void()> func) const;
        void set_should_close(std::function<bool()> func) const;

        bool is_running() const;

        void rendering_loop() const;
        void physics_loop() const;


        GameLoopConfig config_;

        struct GameLoopData;
        std::unique_ptr<GameLoopData> data_;
    };
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
