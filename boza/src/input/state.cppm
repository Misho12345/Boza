module boza.input:state;

import :keys;

import std;
import boza.common;

namespace boza::input
{
    constexpr double double_click_timeout = 0.3;

    struct KeyState
    {
        bool is_pressed() const { return data_ & 0x01; }
        bool is_held() const { return data_ & 0x02; }

        void set_pressed(const bool value)
        {
            if (value) data_ |= 0x01;
            else data_ &= ~0x01;
        }

        void set_held(const bool value)
        {
            if (value) data_ |= 0x02;
            else data_ &= ~0x02;
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

    class CallbackExecutor
    {
    public:
        static CallbackExecutor& instance();

        void execute(std::function<void()> func);
        void execute(std::function<void(glm::vec2)> func, glm::vec2 xy);

        ~CallbackExecutor();

    private:
        CallbackExecutor();

        std::vector<std::thread> workers_;
        std::queue<std::function<void()>> tasks_;
        std::mutex queue_mutex_;
        std::condition_variable condition_;
        bool stop_{ false };
    };

    inline void async_execute(std::function<void()> func) { CallbackExecutor::instance().execute(std::move(func)); }
    inline void async_execute(std::function<void(glm::vec2)> func, const glm::vec2 xy)
    {
        CallbackExecutor::instance().execute(std::move(func), xy);
    }

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