export module boza.ecs:system_registry;

import std;
import <flecs.h>;
import :system_common;

namespace boza
{
    namespace app { class GameLoop; }

    flecs::entity_t to_underlying_phase(Phase phase);

    export class SystemRegistry final
    {
        static SystemRegistry& instance();

        void register_stage(SystemStageInfo& stage_info);
        [[nodiscard]] float resolve_interval(Phase phase, float configured_interval) const;

        void initialize_all(const flecs::world& world, float physics_update_rate);

        void call_engine_begin_stages() const;
        void call_startup_stages() const;
        void call_destroy_stages() const;

        static constexpr std::size_t phase_count = static_cast<std::size_t>(Phase::None) + 1;

        SystemRegistry() = default;

        void initialize_phases(const flecs::world& world);
        void initialize_lifecycle_pipelines(const flecs::world& world);
        void create_all_systems() const;
        void resolve_all_configs() const;
        void apply_all_ordering() const;

        void run_stage_phase(Phase phase) const;
        void run_stage_phases(std::span<const Phase> phases) const;

        std::vector<std::reference_wrapper<SystemStageInfo>> stages_{};
        std::array<flecs::entity_t, phase_count> lifecycle_pipelines_{};

        float physics_interval_{ 0.0f };

        mutable bool engine_begin_stages_completed_{ false };
        mutable bool startup_stages_completed_{ false };
        mutable bool destroy_stages_completed_{ false };

        bool initialized_{ false };

        template<typename, Phase, typename...>
        friend class SystemStage;

        friend class app::GameLoop;
    };
}
