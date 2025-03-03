#pragma once
#include "System.hpp"
#include "Core/JobSystem/JobSystem.hpp"

namespace boza
{
    class BOZA_API RenderingSystem final : public System<RenderingSystem, 240, false>
    {
        void on_begin() override;
        void on_iteration() override;
        void on_end() override;

        std::vector<JobSystem::task_id> tasks;
    };
}
