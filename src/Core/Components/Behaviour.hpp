#pragma once
#include "Component.hpp"
#include "Transform.hpp"
#include "../../pch.hpp"

namespace boza
{
    struct BOZA_API Behaviour : Component
    {
        friend class GameObject;

        virtual void start() {}
        virtual void update() {}
        virtual void fixed_update() {}
        virtual void late_update() {}

        [[nodiscard]]
        inline Transform& get_transform() const;

    protected:
        virtual bool parallelize_start() const { return false; }
        virtual bool parallelize_update() const { return false; }
        virtual bool parallelize_fixed_update() const { return false; }
        virtual bool parallelize_late_update() const { return false; }

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
