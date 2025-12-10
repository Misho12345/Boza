module;

#include "api.hpp"
#include <GLFW/glfw3.h>

module boza.input;

import :state;

namespace boza
{
    using namespace input;

    GLFWwindow* window_handle_{ nullptr };

    bool all_keys_held(const KeyCombo& combo, const std::unordered_map<Key, KeyState>& key_states)
    {
        for (const Key k : combo.keys)
        {
            auto it = key_states.find(k);
            if (it == key_states.end() || !it->second.is_held()) return false;
        }

        return true;
    }

    bool any_combo_held(const KeyBinding& binding, const std::unordered_map<Key, KeyState>& key_states)
    {
        for (const KeyCombo& combo : binding.combos)
        {
            if (all_keys_held(combo, key_states)) return true;
        }

        return false;
    }

    void trigger_bindings(const std::vector<BindingEvent>& bindings, const std::unordered_map<Key, KeyState>& key_states)
    {
        for (const auto& [binding, callback] : bindings)
        {
            if (any_combo_held(binding, key_states)) async_execute(callback);
        }
    }

    void on_key_callback(GLFWwindow*, int key_code, int, const int action, int)
    {
        auto&        state     = InputState::instance();
        const auto   key       = static_cast<Key>(key_code);
        auto&        key_state = state.key_states[key];
        const double time      = glfwGetTime();

        if (action == GLFW_PRESS)
        {
            const bool was_held = key_state.is_held();

            if (time - key_state.last_press_time < double_click_timeout)
            {
                const auto it = state.double_click_events.find(key);
                if (it != state.double_click_events.end()) async_execute(it->second);
                if (!state.double_click_bindings.empty()) trigger_bindings(state.double_click_bindings, state.key_states);
            }

            if (!was_held)
            {
                key_state.set_pressed(true);
                key_state.last_press_time = time;

                auto it = state.press_events.find(key);
                if (it != state.press_events.end()) async_execute(it->second);
                if (!state.press_bindings.empty()) trigger_bindings(state.press_bindings, state.key_states);
            }
            else key_state.set_pressed(false);

            key_state.set_held(true);
        }
        else if (action == GLFW_RELEASE)
        {
            key_state.set_pressed(false);
            key_state.set_held(false);

            const auto it = state.release_events.find(key);
            if (it != state.release_events.end()) async_execute(it->second);
            if (!state.release_bindings.empty()) trigger_bindings(state.release_bindings, state.key_states);
        }
    }

    void on_mouse_button_callback(GLFWwindow* window, const int button, const int action, const int mods)
    {
        on_key_callback(window, button, 0, action, mods);
    }

    void on_scroll_callback(GLFWwindow*, const double x, const double y)
    {
        const auto& state = InputState::instance();
        for (const auto& callback : state.mouse_scroll_callbacks)
        {
            async_execute(callback, { x, y });
        }
    }

    void on_cursor_pos_callback(GLFWwindow*, const double x, const double y)
    {
        auto& state = InputState::instance();

        const glm::vec2 current_pos{ static_cast<float>(x), static_cast<float>(y) };

        if (state.first_cursor_move)
        {
            state.last_cursor_pos = current_pos;
            state.first_cursor_move = false;
            return;
        }

        const glm::vec2 delta = current_pos - state.last_cursor_pos;
        state.last_cursor_pos = current_pos;

        for (const auto& callback : state.mouse_move_callbacks)
        {
            async_execute(callback, delta);
        }
    }

    template<Action A> requires (A != Action::MouseMove && A != Action::MouseScroll)
    void Input::on(KeyBinding binding, std::function<void()> callback)
    {
        auto&           state = InputState::instance();
        std::lock_guard lock{ state.mutex };

        BindingEvent event{ std::move(binding), std::move(callback) };

        if constexpr (A == Action::Press) state.press_bindings.push_back(std::move(event));
        else if constexpr (A == Action::Release) state.release_bindings.push_back(std::move(event));
        else if constexpr (A == Action::Hold) state.hold_bindings.push_back(std::move(event));
        else if constexpr (A == Action::DoubleClick) state.double_click_bindings.push_back(std::move(event));
    }

    template<Action A> requires (A != Action::MouseMove && A != Action::MouseScroll)
    void Input::on(KeyCombo combo, std::function<void()> callback)
    {
        on<A>(KeyBinding{ std::move(combo) }, std::move(callback));
    }

    template<Action A> requires (A != Action::MouseMove && A != Action::MouseScroll)
    void Input::on(const Key key, std::function<void()> callback)
    {
        auto&           state = InputState::instance();
        std::lock_guard lock{ state.mutex };

        if constexpr (A == Action::Press) state.press_events.insert_or_assign(key, std::move(callback));
        else if constexpr (A == Action::Release) state.release_events.insert_or_assign(key, std::move(callback));
        else if constexpr (A == Action::Hold) state.hold_events.insert_or_assign(key, std::move(callback));
        else if constexpr (A == Action::DoubleClick) state.double_click_events.insert_or_assign(key, std::move(callback));
    }

    template<Action A> requires (A == Action::MouseMove || A == Action::MouseScroll)
    void Input::on(std::function<void(glm::vec2)> callback)
    {
        auto&           state = InputState::instance();
        std::lock_guard lock{ state.mutex };

        if constexpr (A == Action::MouseMove) state.mouse_move_callbacks.push_back(std::move(callback));
        else state.mouse_scroll_callbacks.push_back(std::move(callback));
    }

    bool Input::is_pressed(const Key key)
    {
        auto& state = InputState::instance();
        const auto it = state.key_states.find(key);
        return it != state.key_states.end() && it->second.is_pressed();
    }

    bool Input::is_held(const Key key)
    {
        auto& state = InputState::instance();
        const auto it = state.key_states.find(key);
        return it != state.key_states.end() && it->second.is_held();
    }

    void Input::init(void* window_handle)
    {
        window_handle_ = static_cast<GLFWwindow*>(window_handle);

        glfwSetKeyCallback(window_handle_, on_key_callback);
        glfwSetMouseButtonCallback(window_handle_, on_mouse_button_callback);
        glfwSetScrollCallback(window_handle_, on_scroll_callback);
        glfwSetCursorPosCallback(window_handle_, on_cursor_pos_callback);
    }

    void Input::update()
    {
        auto& state = InputState::instance();

        if (state.hold_events.empty() && state.hold_bindings.empty()) return;

        for (const auto& [key, key_state] : state.key_states)
        {
            if (key_state.is_held())
            {
                auto it = state.hold_events.find(key);
                if (it != state.hold_events.end()) async_execute(it->second);
            }
        }

        if (!state.hold_bindings.empty())
        {
            trigger_bindings(state.hold_bindings, state.key_states);
        }
    }

    void Input::shutdown()
    {
        if (window_handle_)
        {
            glfwSetKeyCallback(window_handle_, nullptr);
            glfwSetMouseButtonCallback(window_handle_, nullptr);
            glfwSetScrollCallback(window_handle_, nullptr);
            glfwSetCursorPosCallback(window_handle_, nullptr);
        }

        InputState::instance().clear();
        window_handle_ = nullptr;
    }

    void Input::reset_cursor_tracking() { InputState::instance().first_cursor_move = true; }

    template BOZA_API void Input::on<Action::Press>(KeyBinding, std::function<void()>);
    template BOZA_API void Input::on<Action::Release>(KeyBinding, std::function<void()>);
    template BOZA_API void Input::on<Action::Hold>(KeyBinding, std::function<void()>);
    template BOZA_API void Input::on<Action::DoubleClick>(KeyBinding, std::function<void()>);

    template BOZA_API void Input::on<Action::Press>(KeyCombo, std::function<void()>);
    template BOZA_API void Input::on<Action::Release>(KeyCombo, std::function<void()>);
    template BOZA_API void Input::on<Action::Hold>(KeyCombo, std::function<void()>);
    template BOZA_API void Input::on<Action::DoubleClick>(KeyCombo, std::function<void()>);

    template BOZA_API void Input::on<Action::Press>(Key, std::function<void()>);
    template BOZA_API void Input::on<Action::Release>(Key, std::function<void()>);
    template BOZA_API void Input::on<Action::Hold>(Key, std::function<void()>);
    template BOZA_API void Input::on<Action::DoubleClick>(Key, std::function<void()>);

    template BOZA_API void Input::on<Action::MouseMove>(std::function<void(glm::vec2)>);
    template BOZA_API void Input::on<Action::MouseScroll>(std::function<void(glm::vec2)>);
}