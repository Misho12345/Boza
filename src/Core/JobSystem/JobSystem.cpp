#include "JobSystem.hpp"

namespace boza
{
    void JobSystem::start()
    {
        instance();
    }

    void JobSystem::stop()
    {
        auto& inst = instance();
        inst.executor.wait_for_all();

        std::lock_guard lock{ inst.mutex };
        for (const auto& task_data : std::views::values(inst.tasks))
            task_data->canceled.store(true);
        inst.tasks.clear();
    }

    JobSystem::task_id JobSystem::push_task(const std::function<void()>& func)
    {
        auto& inst = instance();
        task_id id = inst.next_task_id.fetch_add(1);

        auto task_data = std::make_shared<TaskData>();

        task_data->func     = func;
        task_data->taskflow = std::make_shared<tf::Taskflow>();

        task_data->taskflow->emplace([task_data, id, &inst]
        {
            if (task_data->canceled.load()) return;

            try
            {
                task_data->func();
                task_data->completed.store(true);
            }
            catch (...) { task_data->failed.store(true); }

            std::lock_guard cleanup_lock{ inst.mutex };
            inst.tasks.erase(id);
        });

        {
            std::lock_guard lock{ inst.mutex };
            inst.tasks[id] = task_data;
        }

        inst.executor.run(*task_data->taskflow).get();
        return id;
    }

    bool JobSystem::cancel_task(const task_id id)
    {
        auto& inst = instance();
        std::shared_ptr<TaskData> task_data;

        {
            std::lock_guard lock{ inst.mutex };

            const auto it = inst.tasks.find(id);

            if (it == inst.tasks.end()) return false;
            task_data = it->second;
        }

        task_data->canceled.store(true);
        return true;
    }

    JobError JobSystem::wait_for_task(const task_id id)
    {
        auto& inst = instance();
        std::shared_ptr<TaskData> task_data;

        {
            std::lock_guard lock{ inst.mutex };

            const auto it = inst.tasks.find(id);

            if (it == inst.tasks.end()) return JobError::TaskNotFound;
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


    std::optional<JobError> JobSystem::is_task_completed(const task_id id)
    {
        auto& inst = instance();

        std::shared_ptr<TaskData> task_data;

        {
            std::lock_guard lock{ inst.mutex };

            const auto it = inst.tasks.find(id);

            if (it == inst.tasks.end()) return std::nullopt;
            task_data = it->second;
        }

        if (task_data->completed.load()) return JobError::Success;
        if (task_data->canceled.load()) return JobError::TaskCanceled;
        if (task_data->failed.load()) return JobError::TaskFailed;

        return std::nullopt;
    }
}
