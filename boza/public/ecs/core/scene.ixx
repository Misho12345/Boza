export module boza.ecs:scene;

import <flecs.h>;

import std;
import boza.common;

import :game_object;
import :tags;

namespace boza
{
    class App;
    struct RenderingSystem;
    namespace app { class GameLoop; }
}

namespace boza
{
    export class Scene final
    {
        [[nodiscard]]
        std::string_view get_name() const;
        void             set_name(std::string_view scene_name) const;

        [[nodiscard]]
        bool get_active() const;
        void set_active(bool value) const;

        [[nodiscard]]
        static Scene& get_main_scene();
        static void   set_main_scene(const Scene& scene);

        [[nodiscard]]
        static Scene& get_persistent_scene();

    public:
        Scene() = default;

        static Scene create(std::string_view name, bool active = true);
        void destroy() const;

        Scene(const Scene& other);
        Scene(Scene&& other) noexcept;
        Scene& operator=(const Scene& other);
        Scene& operator=(Scene&& other) noexcept;

        [[nodiscard]] GameObject& root() { return root_; }
        [[nodiscard]] const GameObject& root() const { return root_; }

        [[nodiscard]]
        static Scene get(std::string_view name);

        static inline GlobalProperty<
            &Scene::get_main_scene,
            &Scene::set_main_scene
        > main;

        static inline GlobalProperty<
            &Scene::get_persistent_scene
        > persistent;

        [[msvc::no_unique_address]]
        Property<
            Scene,
            &Scene::get_name,
            &Scene::set_name
        > name{ this };

        [[msvc::no_unique_address]]
        Property<
            Scene,
            &Scene::get_active,
            &Scene::set_active
        > active{ this };

        bool operator==(const Scene& other) const { return root_ == other.root_; }
        bool operator!=(const Scene& other) const { return !(*this == other); }

        [[nodiscard]]
        bool valid() const { return root_.valid(); }

    private:
        explicit Scene(const GameObject& root) : root_{ root } {}

        static void remove_objects_for_destruction();

        static flecs::world& world();
        GameObject root_;

        static Scene main_scene_;

        friend class GameObject;

        friend class App;
        friend class app::GameLoop;

        friend struct RenderingSystem;

        friend class SystemRegistry;

        template <typename, Phase, typename...>
        friend struct SystemStage;

        template <auto...>
        friend class GlobalProperty;
    };

    inline Scene Scene::main_scene_{};
}
