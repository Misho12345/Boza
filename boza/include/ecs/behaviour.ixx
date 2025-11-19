module;

#include "api.hpp"

export module boza.ecs:behaviour;

import :component;

export namespace boza
{
    class BOZA_API Behaviour : public Component
    {
    public:
        ~Behaviour() override = default;

        virtual void awake() {}
        virtual void start() {}

        virtual void update([[maybe_unused]] float dt) {}
        virtual void late_update([[maybe_unused]] float dt) {}
        virtual void fixed_update([[maybe_unused]] float fixed_dt) {}

        virtual void on_destroy() {}

    protected:
        Behaviour() = default;
    };
}
