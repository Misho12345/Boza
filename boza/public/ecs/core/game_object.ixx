export module boza.ecs:game_object;

import :tags;
import :transform;
import :camera;
import :common;

export namespace boza
{
    class Scene;

    class GameObject
    {
        [[nodiscard]]
        bool is_active() const;

        [[nodiscard]]
        bool is_active_self() const;
        void set_active_self(bool obj_active_self) const;

        [[nodiscard]]
        std::string_view get_name() const;
        void             set_name(std::string_view obj_name) const;

        [[nodiscard]]
        GameObject get_parent() const;
        void       set_parent(const GameObject& obj_parent) const;

    public:
        GameObject() = default;

        static GameObject create(std::string_view obj_name, const Scene& obj_scene, bool obj_active = true);
        static GameObject create(std::string_view obj_name, const GameObject& obj_parent, bool obj_active = true);
        static GameObject create(std::string_view obj_name, bool obj_active = true);

        void destroy() const;

        GameObject(const GameObject& other);
        GameObject(GameObject&& other) noexcept;
        GameObject& operator=(const GameObject& other);
        GameObject& operator=(GameObject&& other) noexcept;

        [[nodiscard]] static GameObject find(std::string_view obj_name);
        [[nodiscard]] GameObject        find_child(std::string_view child_name) const;

        [[nodiscard]]
        std::vector<GameObject> children() const;


        template <typename C, typename... Args> C& add_component(Args&&... args);
        template <typename C, typename... Args> C& ensure_component(Args&&... args);

        template <typename C> [[nodiscard]] decltype(auto) get_component(this auto&& self);
        template <typename C> [[nodiscard]] auto try_get_component(this auto&& self);

        template <typename C>
        [[nodiscard]] bool has_component() const;

        template <typename C>
        void remove_component() const;


        template <typename R, typename... Args> R& add_relation(const GameObject& target, Args&&... args);
        template <typename R, typename... Args> R& ensure_relation(const GameObject& target, Args&&... args);

        template <typename R> [[nodiscard]] decltype(auto) get_relation_data(this auto&& self, const GameObject& target);
        template <typename R> [[nodiscard]] auto try_get_relation_data(this auto&& self, const GameObject& target);

        template <typename R>
        [[nodiscard]] bool has_relation(const GameObject& target) const;

        template <typename R>
        void remove_relation(const GameObject& target) const;

        template <typename R>
        [[nodiscard]] GameObject get_relation_target() const;

        template <typename R>
        void for_each_with_relation(auto&& callback) const requires
            std::invocable<decltype(callback)&, GameObject> &&
            !std::invocable<decltype(callback)&, GameObject, R&> &&
            !std::invocable<decltype(callback)&, GameObject, const R&>;

        template <typename R>
        void for_each_with_relation(auto&& callback) const requires
            !std::invocable<decltype(callback)&, GameObject> && (
                std::invocable<decltype(callback)&, GameObject, R&> ||
                std::invocable<decltype(callback)&, GameObject, const R&>);

        void for_each_child(auto&& callback) const requires std::invocable<decltype(callback)&, GameObject>;


        [[msvc::no_unique_address]]
        Property<
            GameObject,
            &GameObject::is_active_self,
            &GameObject::set_active_self
        > active_self{ this };

        [[msvc::no_unique_address]]
        Property<
            GameObject,
            &GameObject::is_active
        > active{ this };

        [[msvc::no_unique_address]]
        Property<
            GameObject,
            &GameObject::get_name,
            &GameObject::set_name
        > name{ this };

        [[msvc::no_unique_address]]
        Property<
            GameObject,
            &GameObject::get_parent,
            &GameObject::set_parent
        > parent{ this };

        [[nodiscard]]
        bool valid() const { return entity_.is_valid(); }

        bool operator==(const GameObject& other) const { return entity_.id() == other.entity_.id(); }
        bool operator!=(const GameObject& other) const { return !(*this == other); }

    protected:
        explicit GameObject(const flecs::entity e) : entity_{ e } {}
        explicit GameObject(std::string_view obj_name, const GameObject& obj_parent, bool obj_active);

        flecs::entity entity_;

    private:
        void update_active_hierarchy(bool parent_should_be_active = true) const;

        friend class Scene;
        template <typename>
        friend struct StageRegistrar;

        template <typename, Phase, typename...>
        friend struct SystemStage;
    };

    template <typename C, typename... Args>
    C& GameObject::add_component(Args&&... args)
    {
        if constexpr (std::is_empty_v<C>)
        {
            (void)entity_.add<C>();
            static C empty_tag;
            return empty_tag;
        }
        else
        {
            entity_.emplace<C>(std::forward<Args>(args)...);
            C& comp = entity_.get_mut<C>();

            if constexpr (requires { std::declval<C&>().entity_ = std::declval<flecs::entity>(); }) comp.entity_ = entity_;
            else if constexpr (requires { std::declval<C&>().entity = std::declval<flecs::entity>(); }) comp.entity = entity_;

            if constexpr (requires { std::declval<C&>().game_object_ = std::declval<GameObject>(); }) comp.game_object_ = *this;
            else if constexpr (requires { std::declval<C&>().game_object = std::declval<GameObject>(); }) comp.game_object = *this;

            return comp;
        }
    }

    template <typename C, typename... Args>
    C& GameObject::ensure_component(Args&&... args)
    {
        if constexpr (std::is_empty_v<C>) {
            if (!entity_.has<C>()) entity_.add<C>();
            static C empty_tag;
            return empty_tag;
        } else {
            if (C* comp = try_get_component<C>()) return *comp;
            return add_component<C>(std::forward<Args>(args)...);
        }
    }


    template <typename C>
    decltype(auto) GameObject::get_component(this auto&& self)
    {
        using Self = std::remove_reference_t<decltype(self)>;
        if constexpr (std::is_const_v<Self>) return self.entity_.template get<C>();
        else return self.entity_.template get_mut<C>();
    }

    template <typename C>
    auto GameObject::try_get_component(this auto&& self)
    {
        using Self = std::remove_reference_t<decltype(self)>;
        if constexpr (std::is_const_v<Self>) return self.entity_.template try_get<C>();
        else return self.entity_.template try_get_mut<C>();
    }


    template <typename C> bool GameObject::has_component() const { return entity_.has<C>(); }
    template <typename C> void GameObject::remove_component() const { (void)entity_.remove<C>(); }



    template <typename R, typename ... Args>
    R& GameObject::add_relation(const GameObject& target, Args&&... args)
    {
        return entity_.emplace<R>(target.entity_, std::forward<Args>(args)...);
    }

    template <typename R, typename ... Args>
    R& GameObject::ensure_relation(const GameObject& target, Args&&... args)
    {
        if (R* rel = try_get_relation_data<R>(target)) return *rel;
        return add_relation<R>(target, std::forward<Args>(args)...);
    }


    template <typename R>
    decltype(auto) GameObject::get_relation_data(this auto&& self, const GameObject& target)
    {
        using Self = std::remove_reference_t<decltype(self)>;
        if constexpr (std::is_const_v<Self>) return self.entity_.template get<R>(target.entity_);
        else return self.entity_.template get_mut<R>(target.entity_);
    }

    template <typename R>
    auto GameObject::try_get_relation_data(this auto&& self, const GameObject& target)
    {
        using Self = std::remove_reference_t<decltype(self)>;
        if constexpr (std::is_const_v<Self>) return self.entity_.template try_get<R>(target.entity_);
        else return self.entity_.template try_get_mut<R>(target.entity_);
    }


    template <typename R>
    bool GameObject::has_relation(const GameObject& target) const { return entity_.has<R>(target.entity_); }

    template <typename R>
    void GameObject::remove_relation(const GameObject& target) const { (void)entity_.remove<R>(target.entity_); }


    template <typename R>
    GameObject GameObject::get_relation_target() const { return GameObject{ entity_.target<R>() }; }



    template <typename R>
    void GameObject::for_each_with_relation(auto&& callback) const requires
        std::invocable<decltype(callback)&, GameObject> &&
        !std::invocable<decltype(callback)&, GameObject, R&> &&
        !std::invocable<decltype(callback)&, GameObject, const R&>
    {
        entity_.each<R>([&callback](const flecs::entity target) { std::invoke(callback, GameObject{ target }); });
    }

    template <typename R>
    void GameObject::for_each_with_relation(auto&& callback) const requires
        !std::invocable<decltype(callback)&, GameObject> && (
            std::invocable<decltype(callback)&, GameObject, R&> ||
            std::invocable<decltype(callback)&, GameObject, const R&>)
    {
        entity_.each<R>([&callback](const flecs::entity target, R& r) { std::invoke(callback, GameObject{ target }, r); });
    }


    void GameObject::for_each_child(auto&& callback) const requires std::invocable<decltype(callback)&, GameObject>
    {
        entity_.children([&callback](const flecs::entity child) { std::invoke(callback, GameObject{ child }); });
    }
}
