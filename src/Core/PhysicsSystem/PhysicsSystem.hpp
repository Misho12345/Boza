#pragma once
#include "boza_pch.hpp"
#include "System.hpp"
#include "Core/JobSystem/JobSystem.hpp"

namespace boza
{
    class BOZA_API PhysicsSystem final : public System<PhysicsSystem, 50>
    {
        void on_iteration() override;
        std::vector<JobSystem::task_id> tasks{ 16 };
    };
}
