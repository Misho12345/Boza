#pragma once
#include "Component.hpp"
#include "Transform.hpp"

namespace boza
{
    struct Behaviour : Component
    {
        friend class GameObject;

        virtual void start() {}
        virtual void update(const duration& dt) {}
        virtual void fixed_update(const duration& dt) {}
        virtual void late_update(const duration& dt) {}

        [[nodiscard]]
        inline Transform& get_transform() const;

    private:
        Transform* transform{ nullptr };
    };

    inline Transform& Behaviour::get_transform() const
    {
        assert(transform != nullptr && "Transform is nullptr.");
        return *transform;
    }

    template <typename T>
    concept behaviour_derived = std::derived_from<T, Behaviour> && !std::same_as<T, Behaviour>;
}
