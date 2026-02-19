module boza.ecs;

import std;
import :scene;
import <flecs.h>;

namespace boza
{
    Scene Scene::create(const std::string_view name, const bool active)
    {
        auto root = GameObject::create(name, GameObject{}, active);
        root.add_component<tags::Scene>();
        return Scene{ root };
    }

    void Scene::destroy() const { root_.destroy(); }

    Scene::Scene(const Scene& other) : root_{ other.root_ } { }
    Scene::Scene(Scene&& other) noexcept : root_{ std::move(other.root_) } {}

    Scene& Scene::operator=(const Scene& other)
    {
        if (this != &other) root_ = other.root_;
        return *this;
    }


    Scene& Scene::operator=(Scene&& other) noexcept
    {
        if (this != &other) root_ = std::move(other.root_);
        return *this;
    }


    Scene Scene::get(const std::string_view name)
    {
        const flecs::entity e = world().lookup(name.data(), "::", "::", false);
        return Scene{ GameObject{ (e.is_valid() && e.has<tags::Scene>() ? e : flecs::entity::null()) } };
    }

    void Scene::remove_objects_for_destruction() { world().delete_with<tags::PendingDestruction>(); }

    flecs::world& Scene::world()
    {
        static flecs::world w;

        static bool initialized = false;
        if (!initialized)
        {
            w.set_threads(std::thread::hardware_concurrency());
            initialized = true;
        }

        return w;
    }


    std::string_view Scene::get_name() const { return root_.get_name(); }
    void Scene::set_name(const std::string_view scene_name) const { root_.set_name(scene_name); }

    bool  Scene::get_active() const { return root_.is_active_self(); }
    void  Scene::set_active(const bool value) const { root_.set_active_self(value); }

    Scene& Scene::get_main_scene() { return main_scene_; }
    void   Scene::set_main_scene(const Scene& scene) { main_scene_ = scene; }

    Scene& Scene::get_persistent_scene()
    {
        static Scene persistent_scene = create("__BozaPersistent__");
        return persistent_scene;
    }
}
