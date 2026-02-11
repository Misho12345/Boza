module;

#include <GLFW/glfw3.h>

module boza.input;

import :input_system;
import :input_capture;

import boza.app;
import boza.rhi.render_context;

namespace boza
{
    static bool all_keys_held(const KeyCombo& combo, const flat_map<Key, KeyState>& key_states)
    {
        for (const Key k : combo.keys)
        {
            auto it = key_states.find(k);
            if (it == key_states.end() || !it->second.is_held()) return false;
        }
        return true;
    }

    static bool any_combo_held(const KeyBinding& binding, const flat_map<Key, KeyState>& key_states)
    {
        for (const KeyCombo& combo : binding.combos)
        {
            if (all_keys_held(combo, key_states)) return true;
        }
        return false;
    }

    static void trigger_bindings(std::span<BindingEvent> bindings, const flat_map<Key, KeyState>& key_states)
    {
        for (auto& [binding, callback] : bindings)
        {
            if (any_combo_held(binding, key_states) && callback) callback();
        }
    }


    void InputSystem::process_press_events(InputCapture& capture, Key key, const flat_map<Key, KeyState>& states)
    {
        if (auto it = capture.press_events_.find(key);
            it != capture.press_events_.end() && it->second)
            it->second();

        if (!capture.press_bindings_.empty()) trigger_bindings(capture.press_bindings_, states);
    }

    void InputSystem::process_release_events(InputCapture& capture, Key key, const flat_map<Key, KeyState>& states)
    {
        if (auto it = capture.release_events_.find(key);
            it != capture.release_events_.end() && it->second)
            it->second();

        if (!capture.release_bindings_.empty()) trigger_bindings(capture.release_bindings_, states);
    }

    void InputSystem::process_double_click_events(InputCapture& capture, Key key, const flat_map<Key, KeyState>& states)
    {
        if (auto it = capture.double_click_events_.find(key);
            it != capture.double_click_events_.end() && it->second)
            it->second();

        if (!capture.double_click_bindings_.empty()) trigger_bindings(capture.double_click_bindings_, states);
    }

    void InputSystem::process_hold_events(InputCapture& capture, const flat_map<Key, KeyState>& states)
    {
        for (const auto& [key, key_state] : states)
        {
            if (!key_state.is_held()) continue;

            if (auto it = capture.hold_events_.find(key);
                it != capture.hold_events_.end() && it->second)
                it->second();
        }

        if (!capture.hold_bindings_.empty()) trigger_bindings(capture.hold_bindings_, states);
    }

    void InputSystem::process_mouse_move(InputCapture& capture, const glm::vec2 delta)
    {
        for (auto& callback : capture.mouse_move_callbacks_)
        {
            if (callback) callback(delta);
        }
    }

    void InputSystem::process_mouse_scroll(InputCapture& capture, const glm::vec2 offset)
    {
        for (auto& callback : capture.mouse_scroll_callbacks_)
        {
            if (callback) callback(offset);
        }
    }


    static void on_key_callback(GLFWwindow*, int key_code, int, const int action, int)
    {
        if (key_code == GLFW_KEY_UNKNOWN) return;

        const auto key = static_cast<Key>(key_code);
        const double time      = glfwGetTime();
        auto& key_state = InputSystem::frame.key_states[key];

        if (action == GLFW_PRESS)
        {
            const bool was_held = key_state.is_held();

            const bool is_double_click =
                key_state.last_press_time > 0.0 &&
                (time - key_state.last_press_time) < double_click_timeout;

            if (is_double_click)
            {
                key_state.last_press_time = 0.0;
                InputSystem::frame.keys_double_clicked.push_back(key);
            }
            else key_state.last_press_time = time;

            key_state.set_held(true);

            if (!was_held)
            {
                key_state.set_pressed(true);
                InputSystem::frame.keys_pressed.push_back(key);
            }
            else key_state.set_pressed(false);
        }
        else if (action == GLFW_RELEASE)
        {
            key_state.set_pressed(false);
            key_state.set_held(false);
            InputSystem::frame.keys_released.push_back(key);
        }
    }

    static void on_mouse_button_callback(GLFWwindow* window, const int button, const int action, const int mods)
    {
        on_key_callback(window, button, 0, action, mods);
    }

    static void on_scroll_callback(GLFWwindow*, const double x, const double y)
    {
        InputSystem::frame.scroll_delta.x += static_cast<float>(x);
        InputSystem::frame.scroll_delta.y += static_cast<float>(y);
    }

    static void on_cursor_pos_callback(GLFWwindow*, const double x, const double y)
    {
        const glm::vec2 current_pos{ static_cast<float>(x), static_cast<float>(y) };

        if (InputSystem::first_cursor_move)
        {
            InputSystem::last_cursor_pos = current_pos;
            InputSystem::first_cursor_move = false;
            return;
        }

        const glm::vec2 delta = current_pos - InputSystem::last_cursor_pos;
        InputSystem::last_cursor_pos = current_pos;
        InputSystem::frame.mouse_delta += delta;
    }


    void InputSystem::Begin::execute()
    {
        auto* glfw_window = static_cast<GLFWwindow*>(rhi::RenderContext::window()->native_handle());
        if (!glfw_window) return;

        glfwSetKeyCallback(glfw_window, on_key_callback);
        glfwSetMouseButtonCallback(glfw_window, on_mouse_button_callback);
        glfwSetScrollCallback(glfw_window, on_scroll_callback);
        glfwSetCursorPosCallback(glfw_window, on_cursor_pos_callback);

        frame.key_states.reserve(64);

        frame.key_states.clear();
        frame.keys_pressed.clear();
        frame.keys_released.clear();
        frame.keys_double_clicked.clear();
        frame.mouse_delta = glm::vec2{ 0.0f, 0.0f };
        frame.scroll_delta = glm::vec2{ 0.0f, 0.0f };

        last_cursor_pos = glm::vec2{ 0.0f, 0.0f };
        first_cursor_move = true;
    }

    void InputSystem::Input::execute()
    {
        for (auto& key_state : frame.key_states | std::views::values)
        {
            key_state.set_pressed(false);
        }

        frame.keys_pressed.clear();
        frame.keys_released.clear();
        frame.keys_double_clicked.clear();
        frame.mouse_delta = glm::vec2{ 0.0f, 0.0f };
        frame.scroll_delta = glm::vec2{ 0.0f, 0.0f };

        platform::Window* window = rhi::RenderContext::window();
        window->poll_events();
        if (window->should_close()) App::quit();
    }

    void InputSystem::Update::execute(InputCapture& capture)
    {
        for (const Key key : frame.keys_pressed) process_press_events(capture, key, frame.key_states);
        for (const Key key : frame.keys_released) process_release_events(capture, key, frame.key_states);
        for (const Key key : frame.keys_double_clicked) process_double_click_events(capture, key, frame.key_states);

        process_hold_events(capture, frame.key_states);

        if (glm::length2(frame.mouse_delta) > 1e-8f) process_mouse_move(capture, frame.mouse_delta);
        if (glm::length2(frame.scroll_delta) > 1e-8f) process_mouse_scroll(capture, frame.scroll_delta);
    }

    void InputSystem::Destroy::execute()
    {
        auto* glfw_window = static_cast<GLFWwindow*>(rhi::RenderContext::window()->native_handle());
        if (!glfw_window) return;

        glfwSetKeyCallback(glfw_window, nullptr);
        glfwSetMouseButtonCallback(glfw_window, nullptr);
        glfwSetScrollCallback(glfw_window, nullptr);
        glfwSetCursorPosCallback(glfw_window, nullptr);

        frame.key_states.clear();
        frame.keys_pressed.clear();
        frame.keys_released.clear();
        frame.keys_double_clicked.clear();
        frame.mouse_delta = glm::vec2{ 0.0f, 0.0f };
        frame.scroll_delta = glm::vec2{ 0.0f, 0.0f };
    }


    bool Input::is_pressed(const Key key)
    {
        const auto it = InputSystem::frame.key_states.find(key);
        return it != InputSystem::frame.key_states.end() && it->second.is_pressed();
    }

    bool Input::is_held(const Key key)
    {
        const auto it = InputSystem::frame.key_states.find(key);
        return it != InputSystem::frame.key_states.end() && it->second.is_held();
    }

    void Input::reset_cursor_tracking()
    {
        InputSystem::first_cursor_move = true;
    }
}
