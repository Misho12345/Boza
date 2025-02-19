#include "Scene.hpp"

#include "GameObject.hpp"
#include "Components/GameObjData.hpp"

namespace boza
{
    Scene::Scene(const std::string& name) : name{ name }
    {
        assert(!scenes.contains(name) && "Scene already exists.");
        if (active_scene == nullptr) active_scene = this;
        scenes.emplace(name, this);
    }

    Scene::~Scene()
    {
        if (active_scene == this) active_scene = nullptr;
        scenes.erase(name);
    }


    std::unordered_set<GameObject*> Scene::get_game_objects() const
    {
        std::unordered_set<GameObject*> game_objects;

        for (const auto& entity : this->game_objects)
        {
            auto data = registry.get<GameObjData>(entity);
            game_objects.emplace(data.game_object);
        }

        return game_objects;
    }


    Scene& Scene::get_active_scene()
    {
        assert(active_scene != nullptr && "No active scene set.");
        return *active_scene;
    }

    Scene& Scene::get(const std::string& name)
    {
        assert(scenes.contains(name) && scenes.at(name) != nullptr && "Scene does not exist.");
        return *scenes.at(name);
    }

    const std::string& Scene::get_name() const { return name; }


    void Scene::push_game_object(const GameObject* game_object)
    {
        assert(game_object != nullptr && "Game object is nullptr.");
        assert(!active_scene->game_objects.contains(game_object->get_id()) && "Game object already exists in scene.");
        active_scene->game_objects.emplace(game_object->get_id());
    }

    void Scene::pop_game_object(const GameObject* game_object)
    {
        assert(game_object != nullptr && "Game object is nullptr.");
        assert(active_scene->game_objects.contains(game_object->get_id()) && "Game object does not exist in scene.");
        active_scene->game_objects.erase(game_object->get_id());
    }
}
