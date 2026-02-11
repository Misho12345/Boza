module;

#include "api.hpp"

export module boza.input:key_state;

import std;

export namespace boza
{
    constexpr double double_click_timeout = 0.3;

    class BOZA_API KeyState final
    {
    public:
        KeyState()  = default;
        ~KeyState() = default;

        [[nodiscard]] bool is_pressed() const { return pressed_; }
        [[nodiscard]] bool is_held() const { return held_; }

        void set_pressed(const bool pressed) { pressed_ = pressed; }
        void set_held(const bool held) { held_ = held; }

        double last_press_time{ 0.0 };

    private:
        bool pressed_{ false };
        bool held_{ false };
    };
}