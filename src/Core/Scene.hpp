#pragma once
#include "boza_pch.hpp"

namespace boza
{
    class GameObject;

    class BOZA_API Scene final
    {
        friend GameObject;

    public:
        explicit Scene(const std::string& name);
        ~Scene();

        [[nodiscard]] const std::string& get_name() const;
        [[nodiscard]] hash_set<GameObject*> get_game_objects() const;

        [[nodiscard]] static Scene& get_active_scene();
        [[nodiscard]] static Scene& get(const std::string& name);

    private:
        static void push_game_object(const GameObject* game_object);
        static void pop_game_object(const GameObject* game_object);

        std::string name;
        hash_set<entt::entity> game_objects;

        static entt::registry& registry();
        static Scene*& active_scene();
        static hash_map<std::string, Scene*>& scenes();
    };
}
