module boza.input;

import :state;

namespace boza::input
{

    void async_execute(std::function<void()> func)
    {
        auto&           state = InputState::instance();
        std::lock_guard lock{ state.execution_mutex };
        state.execution_queue.push(std::move(func));
    }

    void async_execute(std::function<void(glm::vec2)> func, const glm::vec2 xy)
    {
        auto&           state = InputState::instance();
        std::lock_guard lock{ state.execution_mutex };
        state.execution_queue.push([func = std::move(func), xy] { func(xy); });
    }

    void InputState::clear()
    {
        std::lock_guard lock{ mutex };

        press_events.clear();
        release_events.clear();
        hold_events.clear();
        double_click_events.clear();

        press_bindings.clear();
        release_bindings.clear();
        hold_bindings.clear();
        double_click_bindings.clear();

        mouse_move_callbacks.clear();
        mouse_scroll_callbacks.clear();

        key_states.clear();

        last_cursor_pos = glm::vec2{ 0.0f, 0.0f };
        first_cursor_move = true;

        {
            std::lock_guard mouse_lock{ mouse_delta_mutex };
            accumulated_mouse_delta = glm::vec2{ 0.0f, 0.0f };
        }

        {
            std::lock_guard exec_lock{ execution_mutex };
            while (!execution_queue.empty()) execution_queue.pop();
        }
    }

    InputState::InputState()
    {
        press_bindings.reserve(16);
        release_bindings.reserve(16);
        hold_bindings.reserve(16);
        double_click_bindings.reserve(8);

        mouse_move_callbacks.reserve(4);
        mouse_scroll_callbacks.reserve(4);

        key_states.reserve(64);
    }
}