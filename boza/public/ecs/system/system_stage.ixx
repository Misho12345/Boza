export module boza.ecs:system_stage;

import std;
import <flecs.h>;

import :scene;
import :system_common;
import :component_list;
import :system_registry;
import :game_object;

import boza.core;

#ifdef assert
#undef assert
#endif

export namespace boza
{
    template <typename Derived, Phase P, typename... Specs>
    struct SystemStage
    {
        using CList = filter_to_component_list<Specs...>;

        static constexpr Phase phase = P;

    private:
        static flecs::system     create_system();
        static SystemStageConfig resolve_config();
        static void              apply_ordering();

        static SystemStageConfig config() { return {}; }

        static inline auto& stage_info = [] -> SystemStageInfo&
        {
            static SystemStageInfo info
            {
                .phase          = phase,
                .create_system  = &SystemStage::create_system,
                .resolve_config = &SystemStage::resolve_config,
                .apply_ordering = &SystemStage::apply_ordering
            };

            SystemRegistry::instance().register_stage(info);
            return info;
        }();

        template <typename, Phase, typename...>
        friend struct SystemStage;
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
        const auto [multi_threaded, include_disabled, interval] = resolve_config();
        const flecs::entity_t engine_kind = to_underlying_phase(P);

        if constexpr (P == Phase::None) builder.kind(flecs::OnUpdate);
        else if (engine_kind != 0) builder.kind(engine_kind);

        apply_specs_to_builder(builder, static_cast<CList*>(nullptr));

        if (multi_threaded) builder.multi_threaded();
        if (interval > 0.0f) builder.interval(interval);
        if (include_disabled) builder.with(flecs::Disabled).optional();

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

    template <typename Derived, Phase P, typename... Specs>
    void SystemStage<Derived, P, Specs...>::apply_ordering()
    {
        ([]
        {
            if constexpr (ordering_spec<Specs>)
            {
                if constexpr (Specs::is_after) Derived::stage_info.system.depends_on(Specs::type::stage_info.system);
                else Specs::type::stage_info.system.depends_on(Derived::stage_info.system);
            }
        }(), ...);
    }

    template <typename Derived, typename... Specs>
    using EngineBeginStage = SystemStage<Derived, Phase::EngineBegin, Specs...>;

    template <typename Derived, typename... Specs>
    using StartStage = SystemStage<Derived, Phase::Start, Specs...>;

    template <typename Derived, typename... Specs>
    using PostStartStage = SystemStage<Derived, Phase::PostStart, Specs...>;

    template <typename Derived, typename... Specs>
    using EnginePhysicsStage = SystemStage<Derived, Phase::EnginePhysics, Specs...>;

    template <typename Derived, typename... Specs>
    using PhysicsStage = SystemStage<Derived, Phase::Physics, Specs...>;

    template <typename Derived, typename... Specs>
    using EngineUpdateStage = SystemStage<Derived, Phase::EngineUpdate, Specs...>;

    template <typename Derived, typename... Specs>
    using PreUpdateStage = SystemStage<Derived, Phase::PreUpdate, Specs...>;

    template <typename Derived, typename... Specs>
    using UpdateStage = SystemStage<Derived, Phase::Update, Specs...>;

    template <typename Derived, typename... Specs>
    using PostUpdateStage = SystemStage<Derived, Phase::PostUpdate, Specs...>;

    template <typename Derived, typename... Specs>
    using PreRenderStage = SystemStage<Derived, Phase::PreRender, Specs...>;

    template <typename Derived, typename... Specs>
    using EngineRenderStage = SystemStage<Derived, Phase::EngineRender, Specs...>;

    template <typename Derived, typename... Specs>
    using DestroyStage = SystemStage<Derived, Phase::Destroy, Specs...>;

    template <typename Derived, typename... Specs>
    using EngineDestroyStage = SystemStage<Derived, Phase::EngineDestroy, Specs...>;

    template <typename Derived, typename... Specs>
    using StandaloneStage = SystemStage<Derived, Phase::None, Specs...>;
}
