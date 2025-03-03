#include "InputSystem.hpp"
#include "Core/JobSystem/JobSystem.hpp"

namespace boza
{
    constexpr double double_click_timeout = 0.3;

    InputSystem::InputSystem()
    {
        glfwSetKeyCallback(Window::get_glfw_window(), on_key_callback);
        glfwSetMouseButtonCallback(Window::get_glfw_window(), on_mouse_button_callback);
        glfwSetScrollCallback(Window::get_glfw_window(), on_scroll_callback);
        glfwSetCursorPosCallback(Window::get_glfw_window(), on_cursor_pos_callback);
    }


    void InputSystem::on_key_callback(GLFWwindow*, const int key_, int, const int action, int)
    {
        auto& inst =  instance();

        const auto   key   = static_cast<Key>(key_);
        auto&        state = inst.key_states[key];
        const double time  = glfwGetTime();

        if (action == GLFW_PRESS)
        {
            if (key == Key::F11) Window::toggle_fullscreen();

            if (time - state.last_press_time < double_click_timeout && inst.key_double_click_events.contains(key))
                JobSystem::push_task(inst.key_double_click_events.at(key));

            if (!state.is_held())
            {
                state.set_pressed(true);
                state.last_press_time = time;

                if (inst.key_press_events.contains(key)) JobSystem::push_task(inst.key_press_events.at(key));
            }
            else state.set_pressed(false);

            state.set_held(true);

            for (const auto& [keys, callback] : inst.key_combination_events)
            {
                if (std::ranges::all_of(keys, [&](const Key k)
                {
                    return inst.key_states[k].is_held();
                })) JobSystem::push_task(callback);
            }
        }
        else if (action == GLFW_RELEASE)
        {
            state.set_pressed(false);
            state.set_held(false);

            if (inst.key_release_events.contains(key)) JobSystem::push_task(inst.key_release_events.at(key));
        }
    }

    void InputSystem::on_mouse_button_callback(GLFWwindow*, const int button, const int action, int)
    {
        on_key_callback(nullptr, button, 0, action, 0);
    }

    void InputSystem::on_scroll_callback(GLFWwindow*, const double x, const double y)
    {
        for (const auto& callback : instance().mouse_wheel_events)
            JobSystem::push_task([=] { callback(x, y); });
    }

    void InputSystem::on_cursor_pos_callback(GLFWwindow*, const double x, const double y)
    {
        for (const auto& callback : instance().mouse_move_events)
            JobSystem::push_task([=] { callback(x, y); });
    }


    void InputSystem::on(const Key key, const KeyAction action, const std::function<void()>& callback)
    {
        auto&           inst = instance();
        std::lock_guard lock{ inst.mutex };

        switch (action)
        {
            case KeyAction::Press: inst.key_press_events.insert_or_assign(key, callback); break;
            case KeyAction::Release: inst.key_release_events.insert_or_assign(key, callback); break;
            case KeyAction::Hold: inst.key_hold_events.insert_or_assign(key, callback); break;
            case KeyAction::DoubleClick: inst.key_double_click_events.insert_or_assign(key, callback); break;
            default:
                assert(false);
                std::unreachable();
        }
    }

    void InputSystem::on(const std::initializer_list<Key>& combination, const std::function<void()>& callback)
    {
        auto&           inst = instance();
        std::lock_guard lock{ inst.mutex };

        KeyCombinationEvent event;
        event.keys.insert(combination.begin(), combination.end());
        event.callback = callback;
        inst.key_combination_events.emplace_back(std::move(event));
    }


    void InputSystem::on_iteration()
    {
        for (const auto& [key, state] : key_states)
        {
            if (state.is_held() && key_hold_events.contains(key))
                JobSystem::push_task(key_hold_events.at(key));
        }
    }

    bool InputSystem::KeyState::is_pressed() const { return data[0]; }
    bool InputSystem::KeyState::is_held() const { return data[1]; }

    void InputSystem::KeyState::set_pressed(const bool value) { data.set(0, value); }
    void InputSystem::KeyState::set_held(const bool value) { data.set(1, value); }
}
