#pragma once
#include "VariableSystem.hpp"

namespace boza
{
    class BOZA_API RenderingSystem final : public VariableSystem<RenderingSystem>
    {
        void on_begin() override;
        void on_iteration() override;
        void on_end() override;

        friend Singleton;
        RenderingSystem() : VariableSystem(120, true) {}
    };
}
