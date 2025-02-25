#pragma once

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

    class BOZA_API JobSystem final
    {
    public:
        using task_id = uint64_t;

        JobSystem() = delete;
        ~JobSystem() = delete;
        JobSystem(const JobSystem&) = delete;
        JobSystem(JobSystem&&) = delete;
        JobSystem& operator=(const JobSystem&) = delete;
        JobSystem& operator=(JobSystem&&) = delete;

        static void init(size_t num_threads = std::thread::hardware_concurrency());
        static void shutdown();

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

        static tf::Executor& executor(size_t n = 0);
        static std::mutex& tasks_mutex();
        static std::atomic_size_t& next_task_id();
        static std::unordered_map<uint64_t, std::shared_ptr<TaskData>>& tasks();
    };
}
