#pragma once

namespace boza
{
    class BOZA_API JobSystem final
    {
    public:
        JobSystem()                              = delete;
        ~JobSystem()                             = delete;
        JobSystem(const JobSystem&)              = delete;
        JobSystem(JobSystem&&)                   = delete;
        JobSystem& operator=(const JobSystem&)   = delete;
        JobSystem& operator=(JobSystem&&)        = delete;

        using task_id = std::uint64_t;

        static task_id push_task(const std::function<void()>& task);
        static void cancel_task(task_id id);
        static void wait_for_task(task_id id);

        static void execute_task(const std::function<void()>& task);
        static void execute_batch(const std::vector<std::function<void()>>& tasks);

    private:
        struct TaskInfo
        {
            std::atomic_bool cancelled{ false };

            std::promise<void> promise;
            std::future<void>  promise_future{ promise.get_future() };

            tf::Future<void> taskflow_future;
        };

        static hash_map<task_id, std::shared_ptr<TaskInfo>>& tasks();
    };
}
