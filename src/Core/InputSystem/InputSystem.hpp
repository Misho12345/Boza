#pragma once
#include "boza_pch.hpp"
#include "Core/Window.hpp"

namespace boza
{
    class BOZA_API InputSystem final
    {
    public:
        enum class KeyAction
        {
            Press,
            Release,
            Hold,
            DoubleClick
        };

        enum class MouseWheelAction {};
        enum class MouseMoveAction {};

        enum class Key
        {
            A = GLFW_KEY_A,
            B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

            Num0 = GLFW_KEY_0,
            Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

            F1 = GLFW_KEY_F1,
            F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

            Up = GLFW_KEY_UP,
            Down, Left, Right,

            LShift = GLFW_KEY_LEFT_SHIFT,
            RShift = GLFW_KEY_RIGHT_SHIFT,

            LCtrl = GLFW_KEY_LEFT_CONTROL,
            RCtrl = GLFW_KEY_RIGHT_CONTROL,

            LAlt = GLFW_KEY_LEFT_ALT,
            RAlt = GLFW_KEY_RIGHT_ALT,

            LSuper = GLFW_KEY_LEFT_SUPER,
            RSuper = GLFW_KEY_RIGHT_SUPER,

            Esc = GLFW_KEY_ESCAPE,
            CapsLock = GLFW_KEY_CAPS_LOCK,
            Space = GLFW_KEY_SPACE,
            Enter = GLFW_KEY_ENTER,
            Backspace = GLFW_KEY_BACKSLASH,
            Tab = GLFW_KEY_TAB,

            Insert = GLFW_KEY_INSERT,
            Delete = GLFW_KEY_DELETE,
            Home = GLFW_KEY_HOME,
            End = GLFW_KEY_END,
            PageUp = GLFW_KEY_PAGE_UP,
            PageDown = GLFW_KEY_PAGE_DOWN,

            PrintScreen = GLFW_KEY_PRINT_SCREEN,
            ScrollLock = GLFW_KEY_SCROLL_LOCK,
            Pause = GLFW_KEY_PAUSE,

            Dot = GLFW_KEY_PERIOD,
            Comma = GLFW_KEY_COMMA,
            Slash = GLFW_KEY_SLASH,
            Backslash = GLFW_KEY_BACKSLASH,
            Apostrophe = GLFW_KEY_APOSTROPHE,
            Semicolon = GLFW_KEY_SEMICOLON,
            Equal = GLFW_KEY_EQUAL,
            Minus = GLFW_KEY_MINUS,
            LBracket = GLFW_KEY_LEFT_BRACKET,
            RBracket = GLFW_KEY_RIGHT_BRACKET,
            GraveAccent = GLFW_KEY_GRAVE_ACCENT,

            MouseLeft = GLFW_MOUSE_BUTTON_LEFT,
            MouseRight = GLFW_MOUSE_BUTTON_RIGHT,
            MouseMiddle = GLFW_MOUSE_BUTTON_MIDDLE
        };


        InputSystem() = delete;
        ~InputSystem() = delete;
        InputSystem(const InputSystem&) = delete;
        InputSystem(InputSystem&&) = delete;
        InputSystem& operator=(const InputSystem&) = delete;
        InputSystem& operator=(InputSystem&&) = delete;

        static void start();
        static void stop();

        template<typename T>
        requires (std::is_same_v<T, MouseWheelAction> || std::is_same_v<T, MouseMoveAction>)
        static void on(const std::function<void(double, double)>& callback)
        {
            std::lock_guard lock{ mutex() };
            if constexpr (std::is_same_v<T, MouseWheelAction>) mouse_wheel_events().emplace_back(callback);
            else mouse_move_events().emplace_back(callback);
        }

        static void on(Key key, KeyAction action, const std::function<void()>& callback);
        static void on(const std::initializer_list<Key>& combination, const std::function<void()>& callback);

        static void init(const Window& window);

    private:
        using clock = std::chrono::high_resolution_clock;
        using time_point = clock::time_point;
        using duration = std::chrono::microseconds;

        struct KeyCombinationEvent
        {
            hash_set<Key> keys;
            std::function<void()> callback;
        };

        struct KeyState
        {
            bool is_pressed() const;
            bool is_held() const;

            void set_pressed(bool value);
            void set_held(bool value);

            double last_press_time{ 0 };

        private:
            std::bitset<2> data{ 0 };
        };

        static void run();

        static std::mutex& mutex();

        static hash_map<Key, std::function<void()>>& key_press_events();
        static hash_map<Key, std::function<void()>>& key_release_events();
        static hash_map<Key, std::function<void()>>& key_hold_events();
        static hash_map<Key, std::function<void()>>& key_double_click_events();

        static std::vector<KeyCombinationEvent>& key_combination_events();
        static std::vector<std::function<void(double, double)>>& mouse_wheel_events();
        static std::vector<std::function<void(double, double)>>& mouse_move_events();

        static hash_map<Key, KeyState>& key_states();


        static std::thread& thread();
        static std::atomic_bool& stop_flag();


        static constexpr duration fixed_delta_time{ 1ms }; // 1000 FPS
        static constexpr duration max_catch_up_time{ 5ms }; // 5 frames
        inline static duration accumulated_time{ 0 };
    };
}
