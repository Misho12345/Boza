module boza.input;

import :state;

namespace boza::input
{
    CallbackExecutor& CallbackExecutor::instance()
    {
        static CallbackExecutor executor;
        return executor;
    }

    void CallbackExecutor::execute(std::function<void()> func)
    {
        {
            std::lock_guard lock{ queue_mutex_ };
            tasks_.emplace(std::move(func));
        }

        condition_.notify_one();
    }

    void CallbackExecutor::execute(std::function<void(double, double)> func, double x, double y)
    {
        execute([f = std::move(func), x, y] { f(x, y); });
    }

    CallbackExecutor::~CallbackExecutor()
    {
        {
            std::lock_guard lock{ queue_mutex_ };
            stop_ = true;
        }

        condition_.notify_all();
        for (auto& worker : workers_)
        {
            if (worker.joinable()) worker.join();
        }
    }

    CallbackExecutor::CallbackExecutor()
    {
        const auto thread_count = std::max(2u, std::thread::hardware_concurrency() / 4);
        workers_.reserve(thread_count);

        for (unsigned i = 0; i < thread_count; ++i)
        {
            workers_.emplace_back([this]
            {
                while (true)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock lock{ queue_mutex_ };
                        condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

                        if (stop_ && tasks_.empty()) return;

                        if (!tasks_.empty())
                        {
                            task = std::move(tasks_.front());
                            tasks_.pop();
                        }
                    }

                    if (task) task();
                }
            });
        }
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
