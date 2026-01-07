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

        virtual void update() {}
        virtual void late_update() {}
        virtual void fixed_update() {}

    protected:
        Behaviour() = default;

    private:
        bool awake_called_{ false };
        bool start_called_{ false };

        friend class Scene;
    };
}
