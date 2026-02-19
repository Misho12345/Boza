module boza.ecs;

import std;
import <flecs.h>;

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

    flecs::entity_t to_underlying_phase(const Phase phase)
    {
        if (is_shutdown_phase(phase) || phase == Phase::None) return 0;
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
    }

    void SystemRegistry::create_all_systems()
    {
        for (auto& stage_ref : stages_)
        {
            auto& info = stage_ref.get();
            if (!info.create_system || info.system.is_valid()) continue;
            info.system = info.create_system();
            info.create_system = nullptr;
        }
    }

    void SystemRegistry::resolve_all_configs()
    {
        for (auto& stage_ref : stages_)
        {
            auto& info = stage_ref.get();
            if (!info.resolve_config) continue;
            info.config = info.resolve_config();
            info.resolve_config = nullptr;
        }
    }

    void SystemRegistry::initialize_all(const flecs::world& world, const float physics_update_rate)
    {
        if (initialized_) return;

        physics_interval_ = physics_update_rate > 0.0f ? 1.0f / physics_update_rate : 0.0f;

        initialize_phases(world);

        create_all_systems();
        resolve_all_configs();

        for (const auto& stage_ref : stages_)
        {
            apply_dependencies(stage_ref.get());
        }

        engine_begin_stages_completed_ = false;
        startup_stages_completed_ = false;
        destroy_stages_completed_ = false;

        initialized_ = true;
    }

    void SystemRegistry::run_stage_phase(const Phase phase) const
    {
        const std::array phases{ phase };
        run_stage_phases(phases);
    }

    void SystemRegistry::run_stage_phases(const std::span<const Phase> phases) const
    {
        std::vector<SystemStageInfo*> phase_stages{};
        phase_stages.reserve(stages_.size());

        for (const Phase phase : phases)
        {
            for (const auto& stage_ref : stages_)
            {
                auto& info = stage_ref.get();
                if (info.phase != phase || !info.system.is_valid()) continue;
                phase_stages.push_back(&info);
            }
        }

        if (phase_stages.empty()) return;

        std::vector<std::vector<std::size_t>> outgoing_edges(phase_stages.size());
        std::vector<std::size_t> indegree(phase_stages.size(), 0);

        const auto find_stage_index = [&phase_stages](const flecs::system dependency) -> std::optional<std::size_t>
        {
            for (std::size_t i = 0; i < phase_stages.size(); ++i)
            {
                if (phase_stages[i]->system == dependency) return i;
            }

            return std::nullopt;
        };

        const auto add_edge = [&outgoing_edges, &indegree](const std::size_t from, const std::size_t to)
        {
            auto& neighbors = outgoing_edges[from];
            if (std::ranges::find(neighbors, to) != neighbors.end()) return;

            neighbors.push_back(to);
            ++indegree[to];
        };

        for (std::size_t stage_index = 0; stage_index < phase_stages.size(); ++stage_index)
        {
            const auto& stage = *phase_stages[stage_index];

            for (const auto dependency : stage.config.run_after)
            {
                if (const auto dependency_index = find_stage_index(dependency); dependency_index.has_value())
                {
                    add_edge(*dependency_index, stage_index);
                }
            }

            for (const auto dependency : stage.config.run_before)
            {
                if (const auto dependency_index = find_stage_index(dependency); dependency_index.has_value())
                {
                    add_edge(stage_index, *dependency_index);
                }
            }
        }

        std::deque<std::size_t> ready{};
        for (std::size_t i = 0; i < indegree.size(); ++i)
        {
            if (indegree[i] == 0) ready.push_back(i);
        }

        std::vector<std::size_t> execution_order{};
        execution_order.reserve(phase_stages.size());

        while (!ready.empty())
        {
            const std::size_t stage_index = ready.front();
            ready.pop_front();

            execution_order.push_back(stage_index);

            for (const std::size_t dependent_index : outgoing_edges[stage_index])
            {
                if (--indegree[dependent_index] == 0) ready.push_back(dependent_index);
            }
        }

        if (execution_order.size() != phase_stages.size())
        {
            Log::warn("Detected lifecycle stage dependency cycle; falling back to registration order");

            for (std::size_t i = 0; i < phase_stages.size(); ++i)
            {
                if (std::ranges::find(execution_order, i) == execution_order.end()) execution_order.push_back(i);
            }
        }

        for (const std::size_t stage_index : execution_order)
        {
            auto& stage = *phase_stages[stage_index];

            stage.system.enable();
            stage.system.run();
            stage.system.disable();
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

        constexpr std::array startup_phases{ Phase::Start, Phase::PostStart };
        run_stage_phases(startup_phases);

        startup_stages_completed_ = true;
    }

    void SystemRegistry::call_destroy_stages() const
    {
        if (destroy_stages_completed_) return;

        const std::array destroy_phases{ Phase::Destroy, Phase::EngineDestroy };
        run_stage_phases(destroy_phases);

        destroy_stages_completed_ = true;
    }

    void SystemRegistry::apply_dependencies(const SystemStageInfo& info) const
    {
        if (!info.system.is_valid()) return;

        for (auto dependency : info.config.run_after)
        {
            if (!dependency.is_valid() || dependency == info.system) continue;
            info.system.depends_on(dependency);
        }

        for (auto dependency : info.config.run_before)
        {
            if (!dependency.is_valid() || dependency == info.system) continue;
            dependency.depends_on(info.system);
        }
    }
}
