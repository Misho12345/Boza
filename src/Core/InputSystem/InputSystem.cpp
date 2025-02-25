#include "InputSystem.hpp"

#include "Logger.hpp"
#include "Core/JobSystem/JobSystem.hpp"

namespace boza
{
    constexpr double double_click_timeout = 0.3;

    STATIC_VARIABLE_FN(InputSystem::mutex, {})

    STATIC_VARIABLE_FN(InputSystem::key_press_events, {})
    STATIC_VARIABLE_FN(InputSystem::key_release_events, {})
    STATIC_VARIABLE_FN(InputSystem::key_hold_events, {})
    STATIC_VARIABLE_FN(InputSystem::key_double_click_events, {})

    STATIC_VARIABLE_FN(InputSystem::key_combination_events, {})
    STATIC_VARIABLE_FN(InputSystem::mouse_wheel_events, {})
    STATIC_VARIABLE_FN(InputSystem::mouse_move_events, {})

    STATIC_VARIABLE_FN(InputSystem::key_states, {})

    STATIC_VARIABLE_FN(InputSystem::thread, {})
    STATIC_VARIABLE_FN(InputSystem::stop_flag, { false })


    void InputSystem::start()
    {
        thread() = std::thread{ run };
    }

    void InputSystem::stop()
    {
        stop_flag() = true;
        if (thread().joinable()) thread().join();
    }

    void InputSystem::run()
    {
        time_point last_time = clock::now();

        while (!stop_flag().load())
        {
            time_point current_time = clock::now();
            accumulated_time += std::chrono::duration_cast<duration>(current_time - last_time);
            last_time = current_time;

            if (accumulated_time > max_catch_up_time)
            {
                int dropped_steps = (accumulated_time - max_catch_up_time) / fixed_delta_time;
                Logger::warn("Physics system is running behind! Dropping {} steps.", dropped_steps);
                accumulated_time = max_catch_up_time;
            }

            while (accumulated_time >= fixed_delta_time)
            {
                for (const auto& [key, state] : key_states())
                {
                    if (state.is_held() && key_hold_events().contains(key))
                        JobSystem::push_task(key_hold_events().at(key));
                }

                accumulated_time -= fixed_delta_time;
            }

            if (time_point end_time = last_time + fixed_delta_time;
                clock::now() < end_time)
                std::this_thread::sleep_until(end_time);
        }
    }


    void InputSystem::on(const Key key, const KeyAction action, const std::function<void()>& callback)
    {
        std::lock_guard lock{ mutex() };

        switch (action)
        {
            case KeyAction::Press: key_press_events().insert_or_assign(key, callback); break;
            case KeyAction::Release: key_release_events().insert_or_assign(key, callback); break;
            case KeyAction::Hold: key_hold_events().insert_or_assign(key, callback); break;
            case KeyAction::DoubleClick: key_double_click_events().insert_or_assign(key, callback); break;
            default:
                assert(false);
                std::unreachable();
        }
    }

    void InputSystem::on(const std::initializer_list<Key>& combination, const std::function<void()>& callback)
    {
        std::lock_guard lock{ mutex() };

        KeyCombinationEvent event;
        event.keys.insert(combination.begin(), combination.end());
        event.callback = callback;
        key_combination_events().emplace_back(std::move(event));
    }



    void InputSystem::init(const Window& window)
    {
        static auto key_callback = [](GLFWwindow*, const int key_, int, const int action, int)
        {
            const auto key = static_cast<Key>(key_);
            auto& state = key_states()[key];
            const double time = glfwGetTime();

            if (action == GLFW_PRESS)
            {
                if (time - state.last_press_time < double_click_timeout && key_double_click_events().contains(key))
                    JobSystem::push_task(key_double_click_events().at(key));

                if (!state.is_held())
                {
                    state.set_pressed(true);
                    state.last_press_time = time;

                    if (key_press_events().contains(key))
                        JobSystem::push_task(key_press_events().at(key));
                }
                else state.set_pressed(false);

                state.set_held(true);

                for (const auto& [keys, callback] : key_combination_events())
                {
                    if (std::ranges::all_of(keys, [](const Key k) { return key_states()[k].is_held(); }))
                        JobSystem::push_task(callback);
                }
            }
            else if (action == GLFW_RELEASE)
            {
                state.set_pressed(false);
                state.set_held(false);

                if (key_release_events().contains(key))
                    JobSystem::push_task(key_release_events().at(key));
            }
        };

        static auto mouse_button_callback = [](GLFWwindow*, const int button, const int action, int)
        {
            key_callback(nullptr, button, 0, action, 0);
        };

        static auto scroll_callback = [](GLFWwindow*, const double x, const double y)
        {
            for (const auto& callback : mouse_wheel_events())
                JobSystem::push_task(std::bind(callback, x, y));
        };

        static auto cursor_pos_callback = [](GLFWwindow*, const double x, const double y)
        {
            for (const auto& callback : mouse_move_events())
                JobSystem::push_task(std::bind(callback, x, y));
        };

        glfwSetKeyCallback(window.get_glfw_window(), key_callback);
        glfwSetMouseButtonCallback(window.get_glfw_window(), mouse_button_callback);
        glfwSetScrollCallback(window.get_glfw_window(), scroll_callback);
        glfwSetCursorPosCallback(window.get_glfw_window(), cursor_pos_callback);
    }

    bool InputSystem::KeyState::is_pressed() const { return data[0]; }
    bool InputSystem::KeyState::is_held() const { return data[1]; }

    void InputSystem::KeyState::set_pressed(const bool value) { data.set(0, value); }
    void InputSystem::KeyState::set_held(const bool value) { data.set(1, value); }
}
