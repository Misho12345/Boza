#include "JobSystem.hpp"

namespace boza
{
    STATIC_VARIABLE_FN_ARGS(tf::Executor, JobSystem::executor, (const size_t n), (n), {
        static bool initialized = n != 0;
        assert(initialized && "JobSystem::init() must be called before using the JobSystem");
    })

    STATIC_VARIABLE_FN(JobSystem::tasks_mutex, {})
    STATIC_VARIABLE_FN(JobSystem::next_task_id, {})
    STATIC_VARIABLE_FN(JobSystem::tasks, {})

    void JobSystem::init(const size_t num_threads)
    {
        executor(num_threads);
    }

    void JobSystem::shutdown()
    {
        executor().wait_for_all();

        std::lock_guard lock{ tasks_mutex() };
        for (const auto& task_data : std::views::values(tasks()))
            task_data->canceled.store(true);
        tasks().clear();
    }

    JobSystem::task_id JobSystem::push_task(const std::function<void()>& func)
    {
        task_id id = next_task_id().fetch_add(1);

        auto task_data = std::make_shared<TaskData>();

        task_data->func     = func;
        task_data->taskflow = std::make_shared<tf::Taskflow>();

        task_data->taskflow->emplace([task_data, id]
        {
            if (task_data->canceled.load()) return;

            try
            {
                task_data->func();
                task_data->completed.store(true);
            }
            catch (...) { task_data->failed.store(true); }

            std::lock_guard cleanup_lock{ tasks_mutex() };
            tasks().erase(id);
        });

        {
            std::lock_guard lock{ tasks_mutex() };
            tasks()[id] = task_data;
        }

        executor().run(*task_data->taskflow).get();
        return id;
    }

    bool JobSystem::cancel_task(const task_id id) {
        std::shared_ptr<TaskData> task_data;

        {
            std::lock_guard lock{ tasks_mutex() };

            const auto it = tasks().find(id);

            if (it == tasks().end()) return false;
            task_data = it->second;
        }

        task_data->canceled.store(true);
        return true;
    }

    JobError JobSystem::wait_for_task(const task_id id)
    {
        std::shared_ptr<TaskData> task_data;

        {
            std::lock_guard lock{ tasks_mutex() };

            const auto it = tasks().find(id);

            if (it == tasks().end()) return JobError::TaskNotFound;
            task_data = it->second;
        }

        while (
            !task_data->completed.load() &&
            !task_data->canceled.load() &&
            !task_data->failed.load())
            std::this_thread::yield();

        if (task_data->canceled.load()) return JobError::TaskCanceled;
        if (task_data->failed.load()) return JobError::TaskFailed;

        return JobError::Success;
    }


    JobError JobSystem::execute_task(const std::function<void()>& func)
    {
        return wait_for_task(push_task(func));
    }

    JobError JobSystem::execute_batch(const std::vector<std::function<void()>>& funcs)
    {
        std::vector<task_id> ids;
        ids.reserve(funcs.size());
        auto firstError = JobError::Success;

        for (const auto& func : funcs)
            ids.push_back(push_task(func));

        for (const auto& id : ids)
        {
            if (const JobError result = wait_for_task(id);
                result != JobError::Success && firstError == JobError::Success)
                firstError = result;
        }

        return firstError;
    }


    std::optional<JobError> JobSystem::is_task_completed(const task_id id) {
        std::shared_ptr<TaskData> task_data;

        {
            std::lock_guard lock{ tasks_mutex() };

            const auto it = tasks().find(id);

            if (it == tasks().end()) return std::nullopt;
            task_data = it->second;
        }

        if (task_data->completed.load()) return JobError::Success;
        if (task_data->canceled.load()) return JobError::TaskCanceled;
        if (task_data->failed.load()) return JobError::TaskFailed;

        return std::nullopt;
    }
}
