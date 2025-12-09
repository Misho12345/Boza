module;

#include "api.hpp"

export module boza.input;

export import :keys;

import std;

namespace boza
{
    class App;

    namespace app { class GameLoop; }

    export class BOZA_API Input final
    {
    public:
        Input()  = delete;
        ~Input() = delete;

        template<Action A> requires (A != Action::MouseMove && A != Action::MouseScroll)
        static void on(KeyBinding binding, std::function<void()> callback);

        template<Action A> requires (A != Action::MouseMove && A != Action::MouseScroll)
        static void on(KeyCombo combo, std::function<void()> callback);

        template<Action A> requires (A != Action::MouseMove && A != Action::MouseScroll)
        static void on(Key key, std::function<void()> callback);

        template<Action A> requires (A == Action::MouseMove || A == Action::MouseScroll)
        static void on(std::function<void(double, double)> callback);

        static bool is_pressed(Key key);
        static bool is_held(Key key);

    private:
        static void init(void* window_handle);
        static void update();
        static void shutdown();

        friend class App;
        friend class app::GameLoop;
    };
}
