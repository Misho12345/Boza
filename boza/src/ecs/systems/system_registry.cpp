module boza.ecs;

import std;
import <flecs.h>;

import :scene;
import :system_registry;
import :transform_system;

#ifdef assert
#undef assert
#endif

namespace boza
{
    constexpr std::size_t phase_count = static_cast<std::size_t>(Phase::None) + 1;
    static std::array<flecs::entity_t, phase_count> phase_entities{};

    static constexpr std::size_t phase_index(const Phase phase)
    {
        return static_cast<std::size_t>(phase);
    }

    static flecs::entity_t create_lifecycle_pipeline(const flecs::world& world, const Phase phase)
    {
        if (!is_lifecycle_phase(phase)) return 0;

        const flecs::entity_t phase_entity = to_underlying_phase(phase);
        if (phase_entity == 0) return 0;

        return world.pipeline()
            .with(flecs::System)
            .with(phase_entity)
            .with(flecs::DependsOn)
                .src()
                .cascade(flecs::DependsOn)
                .optional()
            .build()
            .id();
    }

    flecs::entity_t to_underlying_phase(const Phase phase)
    {
        if (phase == Phase::None) return 0;
        return phase_entities[phase_index(phase)];
    }


    SystemRegistry& SystemRegistry::instance()
    {
        static SystemRegistry registry;
        return registry;
    }

    void SystemRegistry::register_stage(SystemStageInfo& stage_info)
    {
        const auto exists = std::ranges::any_of(stages_, [&stage_info](const auto& stage)
        {
            return &stage.get() == &stage_info;
        });

        if (!exists) stages_.emplace_back(stage_info);
    }

    float SystemRegistry::resolve_interval(const Phase phase, const float configured_interval) const
    {
        if (configured_interval > 0.0f) return configured_interval;

        if (is_physics_phase(phase))
        {
            assert(physics_interval_ > 0.0f, "Physics update rate must be positive");
            return physics_interval_;
        }

        return 0.0f;
    }

    void SystemRegistry::initialize_phases(const flecs::world& world)
    {
        const auto make_phase = [&world](const char* name, const flecs::entity_t depends_on = 0)
        {
            auto phase = world.entity(name).add(flecs::Phase);
            if (depends_on != 0) phase.depends_on(depends_on);
            return phase;
        };

        phase_entities[phase_index(Phase::EngineBegin)] = make_phase("EngineBegin", flecs::OnLoad);
        phase_entities[phase_index(Phase::Start)] = make_phase("Start", to_underlying_phase(Phase::EngineBegin));
        phase_entities[phase_index(Phase::PostStart)] = make_phase("PostStart", to_underlying_phase(Phase::Start));

        phase_entities[phase_index(Phase::EnginePhysics)] = make_phase("EnginePhysics", to_underlying_phase(Phase::PostStart));
        phase_entities[phase_index(Phase::Physics)] = make_phase("Physics", to_underlying_phase(Phase::EnginePhysics));

        phase_entities[phase_index(Phase::EngineUpdate)] = make_phase("EngineUpdate", to_underlying_phase(Phase::PostStart));
        phase_entities[phase_index(Phase::PreUpdate)] = make_phase("PreUpdate", to_underlying_phase(Phase::EngineUpdate));
        phase_entities[phase_index(Phase::Update)] = make_phase("Update", to_underlying_phase(Phase::PreUpdate));
        phase_entities[phase_index(Phase::PostUpdate)] = make_phase("PostUpdate", to_underlying_phase(Phase::Update));

        phase_entities[phase_index(Phase::PreRender)] = make_phase("PreRender", to_underlying_phase(Phase::PostUpdate));
        phase_entities[phase_index(Phase::EngineRender)] = make_phase("EngineRender", to_underlying_phase(Phase::PreRender));

        phase_entities[phase_index(Phase::Destroy)] = make_phase("Destroy", to_underlying_phase(Phase::EngineRender));
        phase_entities[phase_index(Phase::EngineDestroy)] = make_phase("EngineDestroy", to_underlying_phase(Phase::Destroy));
    }

    void SystemRegistry::create_all_systems() const
    {
        for (std::size_t i = 0; i < stages_.size(); ++i)
        {
            auto& info = stages_[i].get();
            if (!info.create_system || info.system.is_valid()) continue;
            info.system = info.create_system();
            info.create_system = nullptr;
        }
    }

    void SystemRegistry::initialize_lifecycle_pipelines(const flecs::world& world)
    {
        static constexpr std::array lifecycle_phases{
            Phase::EngineBegin,
            Phase::Start,
            Phase::PostStart,
            Phase::Destroy,
            Phase::EngineDestroy
        };

        for (const Phase phase : lifecycle_phases)
        {
            lifecycle_pipelines_[phase_index(phase)] = create_lifecycle_pipeline(world, phase);
        }
    }

    void SystemRegistry::resolve_all_configs() const
    {
        for (std::size_t i = 0; i < stages_.size(); ++i)
        {
            auto& info = stages_[i].get();
            if (!info.resolve_config) continue;
            info.config = info.resolve_config();
            info.resolve_config = nullptr;
        }
    }

    void SystemRegistry::apply_all_ordering() const
    {
        for (std::size_t i = 0; i < stages_.size(); ++i)
        {
            const auto& info = stages_[i].get();
            if (info.apply_ordering) info.apply_ordering();
        }
    }

    void SystemRegistry::initialize_all(const flecs::world& world, const float physics_update_rate)
    {
        if (initialized_) return;

        physics_interval_ = physics_update_rate > 0.0f ? 1.0f / physics_update_rate : 0.0f;

        initialize_phases(world);

        constexpr std::size_t max_registration_passes = 64;
        for (std::size_t pass = 0; pass < max_registration_passes; ++pass)
        {
            const std::size_t size_before = stages_.size();

            create_all_systems();
            resolve_all_configs();
            apply_all_ordering();

            if (stages_.size() == size_before) break;
            if (pass + 1 == max_registration_passes)
            {
                Log::warn("System stage registration exceeded stabilization pass budget");
            }
        }

        initialize_lifecycle_pipelines(world);

        engine_begin_stages_completed_ = false;
        startup_stages_completed_ = false;
        destroy_stages_completed_ = false;

        initialized_ = true;
    }

    void SystemRegistry::run_stage_phase(const Phase phase) const
    {
        run_stage_phases({ &phase, 1 });
    }

    void SystemRegistry::run_stage_phases(const std::span<const Phase> phases) const
    {
        for (const Phase phase : phases)
        {
            if (!is_lifecycle_phase(phase)) continue;

            const flecs::entity_t pipeline = lifecycle_pipelines_[phase_index(phase)];
            if (pipeline == 0) continue;

            std::vector<std::reference_wrapper<SystemStageInfo>> phase_stages{};
            phase_stages.reserve(stages_.size());

            for (auto& stage_ref : stages_)
            {
                auto& info = stage_ref.get();
                if (info.phase != phase || !info.system.is_valid()) continue;
                info.system.enable();
                phase_stages.emplace_back(info);
            }

            if (!phase_stages.empty()) Scene::world().run_pipeline(pipeline, 0.0f);

            for (auto& stage_ref : phase_stages)
            {
                stage_ref.get().system.disable();
            }
        }
    }

    void SystemRegistry::call_engine_begin_stages() const
    {
        if (engine_begin_stages_completed_) return;

        run_stage_phase(Phase::EngineBegin);
        engine_begin_stages_completed_ = true;
    }

    void SystemRegistry::call_startup_stages() const
    {
        if (startup_stages_completed_) return;

        static constexpr std::array startup_phases{ Phase::Start, Phase::PostStart };
        run_stage_phases(startup_phases);
        startup_stages_completed_ = true;
    }

    void SystemRegistry::call_destroy_stages() const
    {
        if (destroy_stages_completed_) return;

        static constexpr std::array destroy_phases{ Phase::Destroy, Phase::EngineDestroy };
        run_stage_phases(destroy_phases);
        destroy_stages_completed_ = true;
    }
}
