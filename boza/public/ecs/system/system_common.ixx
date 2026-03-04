export module boza.ecs:system_common;

import std;
import <flecs.h>;

export namespace boza
{
    enum class Phase
    {
        EngineBegin, Start, PostStart,
        EnginePhysics, Physics,
        EngineUpdate, PreUpdate, Update, PostUpdate,
        PreRender, EngineRender,
        Destroy, EngineDestroy,
        None
    };

    struct SystemStageConfig final
    {
        bool  multi_threaded{ false };
        bool  include_disabled{ false };
        float interval{ 0.0f };
    };

    struct SystemStageInfo final
    {
        flecs::system       system{};
        SystemStageConfig   config{};
        Phase               phase{};
        flecs::system     (*create_system)(){};
        SystemStageConfig (*resolve_config)(){};
        void              (*apply_ordering)(){};
    };

    constexpr bool is_shutdown_phase(const Phase phase)
    {
        return phase == Phase::Destroy || phase == Phase::EngineDestroy;
    }

    constexpr bool is_startup_phase(const Phase phase)
    {
        return phase == Phase::Start || phase == Phase::PostStart;
    }

    constexpr bool is_lifecycle_phase(const Phase phase)
    {
        return phase == Phase::EngineBegin || is_startup_phase(phase) || is_shutdown_phase(phase);
    }

    constexpr bool is_physics_phase(const Phase phase)
    {
        return phase == Phase::EnginePhysics || phase == Phase::Physics;
    }
}
