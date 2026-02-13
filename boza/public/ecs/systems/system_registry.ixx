export module boza.ecs:system_registry;

import std;
import <flecs.h>;
import :common;

namespace boza
{
    flecs::entity_t to_underlying_phase(Phase phase);

    export class SystemRegistry final
    {
    public:
        static SystemRegistry& instance();

        void register_stage(SystemStageInfo& stage_info);
        [[nodiscard]] float resolve_interval(Phase phase, float configured_interval) const;

        void initialize_all(const flecs::world& world, float physics_update_rate);

        void call_engine_begin_stages() const;
        void call_startup_stages() const;

        void call_destroy_stages() const;

    private:
        SystemRegistry() = default;

        void initialize_phases(const flecs::world& world);
        void create_all_systems();
        void resolve_all_configs();

        void run_stage_phase(Phase phase) const;
        void run_stage_phases(std::span<const Phase> phases) const;

        void apply_dependencies(const SystemStageInfo& info) const;

        std::vector<std::reference_wrapper<SystemStageInfo>> stages_{};

        float physics_interval_{ 0.0f };

        mutable bool engine_begin_stages_completed_{ false };
        mutable bool startup_stages_completed_{ false };
        mutable bool destroy_stages_completed_{ false };

        bool initialized_{ false };
    };
}
