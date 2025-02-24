#include "JobSystem.hpp"

namespace boza
{
    static tf::Executor& executor()
    {
        static tf::Executor executor;
        return executor;
    }

    static std::mutex& mutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static std::atomic<JobSystem::task_id>& next_task_id()
    {
        static std::atomic<JobSystem::task_id> next_task_id{ 1 };
        return next_task_id;
    }

    decltype(JobSystem::tasks()) JobSystem::tasks()
    {
        static std::remove_reference_t<decltype(tasks())> tasks;
        return tasks;
    }



    JobSystem::task_id JobSystem::push_task(const std::function<void()>& task)
    {
        const task_id id = next_task_id().fetch_add(1);

        auto task_info = std::make_shared<TaskInfo>();

        {
            std::lock_guard lock{ mutex() };
            tasks()[id] = task_info;
        }

        tf::Taskflow tf;
        tf.emplace([task_info_ptr = task_info, task]() mutable
        {
            if (!task_info_ptr->cancelled.load()) task();
            task_info_ptr->promise.set_value();
        });

        task_info->taskflow_future = executor().run(tf);

        return id;
    }

    void JobSystem::cancel_task(const task_id id)
    {
        std::lock_guard lock{ mutex() };

        auto& tasks_ = tasks();
        if (const auto it = tasks_.find(id);
            it != tasks_.end())
            it->second->cancelled.store(true);
    }

    void JobSystem::wait_for_task(const task_id id)
    {
        std::shared_ptr<TaskInfo> taskInfo;

        {
            std::lock_guard lock{ mutex() };

            auto& tasks_ = tasks();
            if (const auto it = tasks_.find(id);
                it != tasks_.end())
                taskInfo = it->second;
        }

        if (taskInfo)
        {
            taskInfo->promise_future.wait();
            taskInfo->taskflow_future.get();

            std::lock_guard lock{ mutex() };
            tasks().erase(id);
        }
    }

    void JobSystem::execute_task(const std::function<void()>& task)
    {
        const task_id id = push_task(task);
        wait_for_task(id);
    }

    void JobSystem::execute_batch(const std::vector<std::function<void()>>& tasks)
    {
        std::vector<task_id> ids;
        for (const auto& task : tasks) ids.push_back(push_task(task));
        for (const task_id id : ids) wait_for_task(id);
    }
}
