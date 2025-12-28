module;

#include "api.hpp"

export module boza.input;

export import :keys;
export import :cursor_state;

import std;
import boza.common;

namespace boza
{
    class App;

    namespace app { class GameLoop; }
    namespace platform { class Window; }

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
        static void on(std::function<void(glm::vec2)> callback);

        static bool is_pressed(Key key);
        static bool is_held(Key key);

    private:
        static void init(void* window);
        static void update();
        static void shutdown();

        static void reset_cursor_tracking();

        friend class App;
        friend class platform::Window;
        friend class app::GameLoop;
    };
}
