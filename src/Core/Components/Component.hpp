#pragma once

namespace boza
{
    class GameObject;

    struct BOZA_API Component
    {
        GameObject* game_object;
    };

    template <typename T>
    concept component_derived = std::derived_from<T, Component> && !std::same_as<T, Component>;
}
