export module boza.ecs:system_stage;

import std;
import <flecs.h>;

import :scene;
import :common;
import :component_list;
import :system_registry;
import :game_object;

import boza.core;

#ifdef assert
#undef assert
#endif

namespace boza
{
    export template <typename Derived, Phase P, typename... Specs>
    struct SystemStage
    {
        using CList = ComponentList<Specs...>;

        static constexpr Phase phase = P;

        static SystemStageConfig resolve_config();
        static flecs::system     create_system();

        static SystemStageInfo& init_stage_info()
        {
            static SystemStageInfo info
            {
                .system         = flecs::system(),
                .config         = {},
                .phase          = phase,
                .create_system  = &SystemStage::create_system,
                .resolve_config = &SystemStage::resolve_config
            };

            [[maybe_unused]]
            static const bool registrar = []
            {
                SystemRegistry::instance().register_stage(info);
                return true;
            }();

            return info;
        }

        static inline SystemStageInfo& stage_info = init_stage_info();
        static SystemStageConfig config() { return SystemStageConfig{}; }
    };


    template <typename Derived, Phase P, typename... Specs>
    SystemStageConfig SystemStage<Derived, P, Specs...>::resolve_config()
    {
        auto cfg = Derived::config();
        assert(cfg.interval >= 0.0f, "System stage interval cannot be negative");
        cfg.interval = SystemRegistry::instance().resolve_interval(P, cfg.interval);
        if constexpr (is_physics_phase(P))
        {
            assert(cfg.interval > 0.0f, "Physics stages require a positive interval");
        }
        return cfg;
    }

    template <typename Derived, Phase P, typename... Specs>
    flecs::system SystemStage<Derived, P, Specs...>::create_system()
    {
        auto builder = Scene::world().system();
        const auto cfg = resolve_config();
        const flecs::entity_t engine_kind = to_underlying_phase(P);

        if constexpr (P == Phase::None) builder.kind(flecs::OnUpdate);
        else if (engine_kind != 0) builder.kind(engine_kind);

        apply_specs_to_builder(builder, static_cast<CList*>(nullptr));

        if (cfg.multi_threaded)   builder.multi_threaded();
        if (cfg.interval > 0.0f)  builder.interval(cfg.interval);
        if (cfg.include_disabled) builder.with(flecs::Disabled).optional();

        flecs::system system{};

        if constexpr (CList::is_singleton)
        {
            system = builder.run([](flecs::iter& it) { while (it.next()) Derived::execute(); });
        }
        else
        {
            system = builder.run([](flecs::iter& it)
            {
                while (it.next())
                {
                    for (const auto row : it)
                    {
                        invoke_execute<Derived, CList>(it, row, GameObject{ it.entity(row) });
                    }
                }
            });
        }

        if constexpr (is_lifecycle_phase(P)) system.disable();

        return system;
    }


    export template <typename Derived, typename... Specs>
    using EngineBeginStage = SystemStage<Derived, Phase::EngineBegin, Specs...>;

    export template <typename Derived, typename... Specs>
    using StartStage = SystemStage<Derived, Phase::Start, Specs...>;

    export template <typename Derived, typename... Specs>
    using PostStartStage = SystemStage<Derived, Phase::PostStart, Specs...>;

    export template <typename Derived, typename... Specs>
    using EnginePhysicsStage = SystemStage<Derived, Phase::EnginePhysics, Specs...>;

    export template <typename Derived, typename... Specs>
    using PhysicsStage = SystemStage<Derived, Phase::Physics, Specs...>;

    export template <typename Derived, typename... Specs>
    using EngineUpdateStage = SystemStage<Derived, Phase::EngineUpdate, Specs...>;

    export template <typename Derived, typename... Specs>
    using PreUpdateStage = SystemStage<Derived, Phase::PreUpdate, Specs...>;

    export template <typename Derived, typename... Specs>
    using UpdateStage = SystemStage<Derived, Phase::Update, Specs...>;

    export template <typename Derived, typename... Specs>
    using PostUpdateStage = SystemStage<Derived, Phase::PostUpdate, Specs...>;

    export template <typename Derived, typename... Specs>
    using PreRenderStage = SystemStage<Derived, Phase::PreRender, Specs...>;

    export template <typename Derived, typename... Specs>
    using EngineRenderStage = SystemStage<Derived, Phase::EngineRender, Specs...>;

    export template <typename Derived, typename... Specs>
    using DestroyStage = SystemStage<Derived, Phase::Destroy, Specs...>;

    export template <typename Derived, typename... Specs>
    using EngineDestroyStage = SystemStage<Derived, Phase::EngineDestroy, Specs...>;

    export template <typename Derived, typename... Specs>
    using StandaloneStage = SystemStage<Derived, Phase::None, Specs...>;
}
