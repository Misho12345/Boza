export module boza.ecs:scene;

import <flecs.h>;

import std;
import boza.common;

import :game_object;
import :tags;

namespace boza
{
    class App;
    namespace app { class GameLoop; }
    namespace gfx { class RenderingSystem; }
}

export namespace boza
{
    class Scene final
    {
        [[nodiscard]]
        std::string_view get_name() const;
        void             set_name(std::string_view name) const;

        [[nodiscard]]
        bool get_active() const;
        void set_active(bool value) const;

        [[nodiscard]]
        static Scene get_main_scene();
        static void  set_main_scene(const Scene& scene);

        [[nodiscard]]
        static Scene get_persistent_scene();

    public:
        static Scene create(std::string_view name, bool active = true);
        void destroy() const;

        Scene(const Scene& other);
        Scene(Scene&& other) noexcept;
        Scene& operator=(const Scene& other);
        Scene& operator=(Scene&& other) noexcept;

        [[nodiscard]]
        GameObject root() const;

        [[nodiscard]]
        static Scene get(std::string_view name);


        static inline GlobalProperty<
            &Scene::get_main_scene,
            &Scene::set_main_scene
        > main;

        static inline GlobalProperty<
            &Scene::get_persistent_scene
        > persistent;


        [[nodiscard]]
        bool valid() const { return root_.is_valid(); }
        operator bool() const { return valid(); }

    private:
        explicit Scene(const flecs::entity root) : root_{ root } {}

        static void remove_objects_for_destruction();

        static flecs::world& world();
        flecs::entity root_;

        static inline flecs::entity main_scene_;
        static inline flecs::entity persistent_scene_;


        friend class GameObject;

        friend class App;
        friend class app::GameLoop;

        friend class gfx::RenderingSystem;

        friend class SystemRegistry;

        template <typename>
        friend struct StageRegistrar;

        template <typename, Phase, typename...>
        friend struct SystemStage;

        template <auto...>
        friend class GlobalProperty;
    };
}
