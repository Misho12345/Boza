module boza.ecs;

import std;
import :game_object;
import <flecs.h>;

namespace boza
{
    GameObject::GameObject(const std::string_view obj_name, const GameObject& obj_parent, const bool obj_active)
        : GameObject{
            obj_parent.valid()
                ? Scene::world().scope(obj_parent.entity_).entity(obj_name.data())
                : Scene::world().entity(obj_name.data())
        }
    {
        set_active_self(obj_active);
        ensure_component<Transform>().mark_dirty();
    }

    GameObject GameObject::create(const std::string_view obj_name, const Scene& obj_scene, const bool obj_active)
    {
        return create(obj_name, obj_scene.root(), obj_active);
    }

    GameObject GameObject::create(const std::string_view obj_name, const GameObject& obj_parent, const bool obj_active)
    {
        return GameObject{ obj_name, obj_parent, obj_active };
    }

    GameObject GameObject::create(const std::string_view obj_name, const bool obj_active)
    {
        return GameObject{ obj_name, Scene::main().root(), obj_active };
    }

    void GameObject::destroy() const
    {
        (void)entity_.add<tags::PendingDestruction>();
        set_active_self(false);
    }

    GameObject::GameObject(const GameObject& other) : entity_{ other.entity_ } {}
    GameObject::GameObject(GameObject&& other) noexcept : entity_{ std::exchange(other.entity_, {}) } {}

    GameObject& GameObject::operator=(const GameObject& other)
    {
        if (this != &other && entity_ != other.entity_) entity_ = other.entity_;
        return *this;
    }

    GameObject& GameObject::operator=(GameObject&& other) noexcept
    {
        if (this != &other && entity_ != other.entity_) entity_ = std::exchange(other.entity_, {});
        return *this;
    }

    GameObject GameObject::find(const std::string_view obj_name)
    {
        return GameObject{ Scene::main().root().entity_.lookup(obj_name.data()) };
    }

    GameObject GameObject::find_child(const std::string_view child_name) const
    {
        return GameObject{ entity_.lookup(child_name.data()) };
    }

    std::vector<GameObject> GameObject::children() const
    {
        std::vector<GameObject> result;
        entity_.children([&result](const flecs::entity child) { result.push_back(GameObject{ child }); });
        return result;
    }

    bool GameObject::is_active() const { return entity_.enabled(); }
    bool GameObject::is_active_self() const { return !entity_.has<tags::DisabledSelf>(); }

    void GameObject::set_active_self(const bool obj_active_self) const
    {
        if (obj_active_self) (void)entity_.remove<tags::DisabledSelf>();
        else (void)entity_.add<tags::DisabledSelf>();

        update_active_hierarchy(obj_active_self && (!parent().valid() || parent().is_active()));
    }

    std::string_view GameObject::get_name() const
    {
        const flecs::string_view obj_name = entity_.name();
        return std::string_view{ obj_name.c_str(), obj_name.size() };
    }

    void GameObject::set_name(const std::string_view obj_name) const { (void)entity_.set_name(obj_name.data()); }

    GameObject GameObject::get_parent() const { return GameObject{ entity_.parent() }; }
    void       GameObject::set_parent(const GameObject& obj_parent) const
    {
        (void)entity_.child_of(obj_parent.entity_);

        if (!entity_.has<Transform>()) return;

        std::vector<flecs::entity> stack{};
        stack.reserve(64);
        stack.clear();
        stack.push_back(entity_);

        while (!stack.empty())
        {
            const flecs::entity current = stack.back();
            stack.pop_back();
            current.get_mut<Transform>().mark_dirty();
            current.children([&stack](const flecs::entity child) { stack.push_back(child); });
        }
    }

    void GameObject::update_active_hierarchy(const bool should_be_active) const
    {
        if (should_be_active != is_active())
        {
            if (should_be_active) (void)entity_.enable();
            else (void)entity_.disable();
        }

        (void)entity_.add<tags::RenderTransformDirty>();

        for_each_child([=](const GameObject obj) { obj.update_active_hierarchy(should_be_active && obj.is_active_self()); });
    }
}
