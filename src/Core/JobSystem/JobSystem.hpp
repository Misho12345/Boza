#pragma once
#include "boza_pch.hpp"
#include "Singleton.hpp"

namespace boza
{
    enum class JobError
    {
        Success,
        TaskCanceled,
        TaskFailed,
        TaskNotFound,
        SystemShutdown
    };

    class BOZA_API JobSystem final : public Singleton<JobSystem>
    {
    public:
        using task_id = uint64_t;

        static void start();
        static void stop();

        static task_id push_task(const std::function<void()>& func);
        static bool cancel_task(task_id id);
        static JobError wait_for_task(task_id id);

        static JobError execute_task(const std::function<void()>& func);
        static JobError execute_batch(const std::vector<std::function<void()>>& funcs);

        static std::optional<JobError> is_task_completed(task_id id);

    private:
        struct TaskData
        {
            std::function<void()> func;

            std::atomic_bool completed{ false };
            std::atomic_bool canceled{ false };
            std::atomic_bool failed{ false };

            std::shared_ptr<tf::Taskflow> taskflow;
        };

        tf::Executor executor{ std::thread::hardware_concurrency() };
        std::mutex mutex;
        std::atomic_size_t next_task_id;
        std::unordered_map<uint64_t, std::shared_ptr<TaskData>> tasks;

        friend Singleton;
        JobSystem() = default;
    };
}
