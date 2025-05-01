#pragma once
#include "boza_pch.hpp"
#include "FixedSystem.hpp"

namespace boza
{
    class BOZA_API PhysicsSystem final : public FixedSystem<PhysicsSystem>
    {
        void on_iteration() override;

        friend Singleton;
        PhysicsSystem() : FixedSystem{ 50 } {}
    };
}
