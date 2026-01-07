module boza.input:state;

import :keys;

import std;
import boza.common;

namespace boza::input
{
    constexpr double double_click_timeout = 0.3;

    struct KeyState
    {
        [[nodiscard]] bool is_pressed() const { return data_ & 0b01; }
        [[nodiscard]] bool is_held() const { return data_ & 0b10; }

        void set_pressed(const bool value)
        {
            if (value) data_ |= 0b01;
            else data_ &= ~0b01;
        }

        void set_held(const bool value)
        {
            if (value) data_ |= 0b10;
            else data_ &= ~0b10;
        }

        double last_press_time{ 0.0 };

    private:
        std::uint8_t data_{ 0 };
    };

    struct BindingEvent
    {
        KeyBinding binding;
        std::function<void()> callback;
    };

    void async_execute(std::function<void()> func);
    void async_execute(std::function<void(glm::vec2)> func, glm::vec2 xy);

    struct InputState
    {
        std::mutex mutex;

        flat_map<Key, std::function<void()>> press_events;
        flat_map<Key, std::function<void()>> release_events;
        flat_map<Key, std::function<void()>> hold_events;
        flat_map<Key, std::function<void()>> double_click_events;

        std::vector<BindingEvent> press_bindings;
        std::vector<BindingEvent> release_bindings;
        std::vector<BindingEvent> hold_bindings;
        std::vector<BindingEvent> double_click_bindings;

        std::vector<std::function<void(glm::vec2)>> mouse_move_callbacks;
        std::vector<std::function<void(glm::vec2)>> mouse_scroll_callbacks;

        flat_map<Key, KeyState> key_states;

        glm::vec2 last_cursor_pos{ 0.0f, 0.0f };
        bool first_cursor_move{ true };

        glm::vec2 accumulated_mouse_delta{ 0.0f, 0.0f };
        std::mutex mouse_delta_mutex;

        std::queue<std::function<void()>> execution_queue;
        std::mutex execution_mutex;

        static InputState& instance()
        {
            static InputState state;
            return state;
        }

        void clear();

    private:
        InputState();
    };
}