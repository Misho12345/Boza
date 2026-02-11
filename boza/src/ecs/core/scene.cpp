module boza.ecs;

import std;
import :scene;
import <flecs.h>;

namespace boza
{
    Scene Scene::create(const std::string_view name, const bool active)
    {
        const flecs::entity e = world().entity(name.data());
        (void)e.add<tags::Scene>();
        GameObject{ e }.set_active_self(active);
        return Scene{ e };
    }

    void Scene::destroy() const
    {
        (void)root_.add<tags::PendingDestruction>();
        set_active(false);
    }

    Scene::Scene(const Scene& other) : root_{ other.root_} { }
    Scene::Scene(Scene&& other) noexcept : root_{ std::exchange(other.root_, {}) } {}

    Scene& Scene::operator=(const Scene& other)
    {
        if (this != &other) root_ = other.root_;
        return *this;
    }


    Scene& Scene::operator=(Scene&& other) noexcept
    {
        if (this != &other) root_ = std::exchange(other.root_, {});
        return *this;
    }


    GameObject Scene::root() const { return GameObject{ root_ };}


    Scene Scene::get(const std::string_view name)
    {
        const flecs::entity e = world().lookup(name.data(), "::", "::", false);
        return Scene{ (e.is_valid() && e.has<tags::Scene>() ? e : flecs::entity::null()) };
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


    std::string_view Scene::get_name() const
    {
        const flecs::string_view name = root_.name();
        return std::string_view{ name.c_str(), name.size() };
    }

    void Scene::set_name(const std::string_view name) const { (void)root_.set_name(name.data()); }

    bool  Scene::get_active() const { return root_.enabled(); }
    void  Scene::set_active(const bool value) const { GameObject{ root_ }.set_active_self(value); }

    Scene Scene::get_main_scene() { return Scene{ main_scene_ }; }
    void  Scene::set_main_scene(const Scene& scene) { main_scene_ = scene.root_; }

    Scene Scene::get_persistent_scene() { return Scene{ persistent_scene_ }; }
}
