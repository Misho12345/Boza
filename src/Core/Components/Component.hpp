#pragma once

namespace boza
{
    class GameObject;

    struct BOZA_API Component
    {
        friend class GameObject;

        virtual ~Component() = default;

        [[nodiscard]]
        inline GameObject& get_game_object() const;

    private:
        GameObject* game_object;
    };

    inline GameObject& Component::get_game_object() const
    {
        assert(game_object != nullptr && "Component has no GameObject assigned!");
        return *game_object;
    }

    template<typename T>
    concept component_derived = std::derived_from<T, Component> && !std::same_as<T, Component>;
}
