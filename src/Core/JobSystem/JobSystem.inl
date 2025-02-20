#pragma once
#include "JobSystem.hpp"

namespace boza
{
    template<typename F, typename... Args>
    auto JobSystem::submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
    {
        using ReturnType = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
           std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        auto job = std::make_shared<Job>([task] { (*task)(); });
        job->dependency_count.store(0);

        ++pending_jobs;
        enqueue_job(job);

        return task->get_future();
    }


    template<typename F, typename... Args>
    auto JobSystem::submit(
        const std::vector<std::shared_future<void>>& dependencies,
        F&& f, Args&&... args) -> std::future<decltype(f(args...))>
    {
        using ReturnType = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
           std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        auto job = std::make_shared<Job>([task] { (*task)(); });
        job->dependency_count.store(dependencies.size());
        ++pending_jobs;

        if (!dependencies.empty())
        {
            auto wait_task = std::make_shared<Job>([this, job, dependencies]
            {
                for (const auto& dep : dependencies) dep.wait();
                job->dependency_count.store(0);
                enqueue_job(job);
            });

            ++pending_jobs;
            enqueue_job(wait_task);
        }
        else enqueue_job(job);

        return task->get_future();
    }
}
