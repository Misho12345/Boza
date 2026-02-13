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
    template <typename Stage>
    concept is_singleton_stage = Stage::CList::is_singleton;

    template <typename Stage>
    struct StageRegistrar
    {
        static SystemStageConfig resolve_config()
        {
            auto cfg = Stage::config();
            assert(cfg.interval >= 0.0f, "System stage interval cannot be negative");
            cfg.interval = SystemRegistry::instance().resolve_interval(Stage::phase, cfg.interval);
            if constexpr (is_physics_phase(Stage::phase))
            {
                assert(cfg.interval > 0.0f, "Physics stages require a positive interval");
            }
            return cfg;
        }

        static flecs::system create_system()
        {
            auto builder = Scene::world().system();
            const auto cfg = resolve_config();
            const flecs::entity_t engine_kind = to_underlying_phase(Stage::phase);

            if constexpr (Stage::phase == Phase::None) builder.kind(flecs::OnUpdate);
            else if (engine_kind != 0) builder.kind(engine_kind);

            apply_specs_to_builder(builder, static_cast<Stage::CList*>(nullptr));

            if (cfg.multi_threaded) builder.multi_threaded();
            if (cfg.interval > 0.0f) builder.interval(cfg.interval);

            flecs::system system{};

            if constexpr (is_singleton_stage<Stage>)
            {
                system = builder.run([](flecs::iter& it) { while (it.next()) Stage::execute(); });
            }
            else
            {
                system = builder.run([](flecs::iter& it)
                {
                    while (it.next())
                    {
                        for (const auto row : it)
                        {
                            invoke_execute<Stage, typename Stage::CList>(it, row, GameObject{ it.entity(row) });
                        }
                    }
                });
            }

            if constexpr (is_lifecycle_phase(Stage::phase)) system.disable();

            return system;
        }
    };


    export template <typename Derived, Phase P, typename... Specs>
    struct SystemStage
    {
        using CList = ComponentList<Specs...>;

        static constexpr Phase phase = P;
        static SystemStageInfo& init_stage_info()
        {
            static SystemStageInfo info
            {
                .system = flecs::system(),
                .config = {},
                .phase = phase,
                .create_system = &StageRegistrar<Derived>::create_system,
                .resolve_config = &StageRegistrar<Derived>::resolve_config
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
