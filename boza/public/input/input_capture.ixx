module;

#include "api.hpp"

export module boza.input:input_capture;

import std;
import boza.common;

import :keys;

export namespace boza
{
    struct BindingEvent
    {
        BindingEvent()                               = default;
        BindingEvent(const BindingEvent&)            = delete;
        BindingEvent& operator=(const BindingEvent&) = delete;
        BindingEvent(BindingEvent&&)                 = default;
        BindingEvent& operator=(BindingEvent&&)      = default;

        KeyBinding                      binding;
        std::move_only_function<void()> callback;
    };

    class BOZA_API InputCapture final
    {
    public:
        InputCapture()                               = default;
        InputCapture(const InputCapture&)            = delete;
        InputCapture& operator=(const InputCapture&) = delete;
        InputCapture(InputCapture&&)                 = default;
        InputCapture& operator=(InputCapture&&)      = default;

        template <Action A> requires (A != Action::MouseMove && A != Action::MouseScroll)
        void on(Key key, std::move_only_function<void()> callback)
        {
            if constexpr (A == Action::Press) press_events_.insert_or_assign(key, std::move(callback));
            else if constexpr (A == Action::Release) release_events_.insert_or_assign(key, std::move(callback));
            else if constexpr (A == Action::Hold) hold_events_.insert_or_assign(key, std::move(callback));
            else if constexpr (A == Action::DoubleClick) double_click_events_.insert_or_assign(key, std::move(callback));
        }

        template <Action A> requires (A != Action::MouseMove && A != Action::MouseScroll)
        void on(KeyCombo combo, std::move_only_function<void()> callback)
        {
            on<A>(KeyBinding{ std::move(combo) }, std::move(callback));
        }

        template <Action A> requires (A != Action::MouseMove && A != Action::MouseScroll)
        void on(KeyBinding binding, std::move_only_function<void()> callback)
        {
            BindingEvent event;
            event.binding = std::move(binding);
            event.callback = std::move(callback);

            if constexpr (A == Action::Press) press_bindings_.push_back(std::move(event));
            else if constexpr (A == Action::Release) release_bindings_.push_back(std::move(event));
            else if constexpr (A == Action::Hold) hold_bindings_.push_back(std::move(event));
            else if constexpr (A == Action::DoubleClick) double_click_bindings_.push_back(std::move(event));
        }

        template <Action A> requires (A == Action::MouseMove)
        void on(std::move_only_function<void(glm::vec2)> callback)
        {
            mouse_move_callbacks_.push_back(std::move(callback));
        }

        template <Action A> requires (A == Action::MouseScroll)
        void on(std::move_only_function<void(glm::vec2)> callback)
        {
            mouse_scroll_callbacks_.push_back(std::move(callback));
        }

        void on_mouse_move(std::move_only_function<void(glm::vec2)> callback)
        {
            mouse_move_callbacks_.push_back(std::move(callback));
        }

        void on_mouse_scroll(std::move_only_function<void(glm::vec2)> callback)
        {
            mouse_scroll_callbacks_.push_back(std::move(callback));
        }

        void clear()
        {
            press_events_.clear();
            release_events_.clear();
            hold_events_.clear();
            double_click_events_.clear();

            press_bindings_.clear();
            release_bindings_.clear();
            hold_bindings_.clear();
            double_click_bindings_.clear();

            mouse_move_callbacks_.clear();
            mouse_scroll_callbacks_.clear();
        }

    private:
        friend struct InputSystem;

        flat_map<Key, std::move_only_function<void()>> press_events_;
        flat_map<Key, std::move_only_function<void()>> release_events_;
        flat_map<Key, std::move_only_function<void()>> hold_events_;
        flat_map<Key, std::move_only_function<void()>> double_click_events_;

        std::vector<BindingEvent> press_bindings_;
        std::vector<BindingEvent> release_bindings_;
        std::vector<BindingEvent> hold_bindings_;
        std::vector<BindingEvent> double_click_bindings_;

        std::vector<std::move_only_function<void(glm::vec2)>> mouse_move_callbacks_;
        std::vector<std::move_only_function<void(glm::vec2)>> mouse_scroll_callbacks_;
    };
}
