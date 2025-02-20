#include "Worker.hpp"
#include "JobSystem.hpp"
#include "Logger.hpp"

namespace boza
{
    Worker::Worker(JobSystem* job_system, const size_t id)
        : job_system{ job_system },
          id{ id },
          worker_thread{ &Worker::run, this } {}

    Worker::~Worker()
    {
        stop();
        if (worker_thread.joinable()) worker_thread.join();
    }

    void Worker::run()
    {
        while (!stop_flag && !job_system->is_shutting_down.load())
        {
            std::shared_ptr<Job> job = pop_job();

            if (!job)
            {
                const size_t workerCount = job_system->get_worker_count();
                for (size_t i = 0; i < workerCount; i++)
                    if ((job = job_system->get_worker((id + i + 1) % workerCount).steal_job())) break;

                if (!job)
                {
                    std::this_thread::yield();
                    continue;
                }
            }

            if (job_system->is_shutting_down.load()) break;

            job->func();

            for (auto& weak_child : job->children)
            {
                if (auto child = weak_child.lock();
                    child && child->dependency_count.fetch_sub(1) == 1)
                    job_system->enqueue_job(child);
            }

            --job_system->pending_jobs;
            job_system->cv.notify_all();
        }
    }

    void Worker::push_job(const std::shared_ptr<Job>& job)
    {
        std::lock_guard lock{ queue_mutex };
        local_queue.push_back(job);
    }

    std::shared_ptr<Job> Worker::pop_job()
    {
        std::lock_guard lock{ queue_mutex };

        if (!local_queue.empty())
        {
            auto job = local_queue.back();
            local_queue.pop_back();
            return job;
        }

        return nullptr;
    }

    std::shared_ptr<Job> Worker::steal_job()
    {
        std::lock_guard lock{ queue_mutex };

        if (!local_queue.empty())
        {
            auto job = local_queue.front();
            local_queue.pop_front();
            return job;
        }

        return nullptr;
    }

    void Worker::stop() { stop_flag = true; }
}
