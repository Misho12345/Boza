#include "JobSystem.hpp"

namespace boza
{
    JobSystem::JobSystem(const size_t num_threads) { resize(num_threads); }

    JobSystem::~JobSystem()
    {
        is_shutting_down = true;
        for (const auto& worker : workers) worker->stop();
        wait();
        workers.clear();
    }

    void JobSystem::enqueue_job(const std::shared_ptr<Job>& job)
    {
        const size_t workerCount = get_worker_count();
        if (workerCount == 0) return;

        thread_local std::mt19937             rng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, workerCount - 1);
        const size_t                          idx = dist(rng);

        workers[idx]->push_job(job);
        cv.notify_all();
    }

    size_t JobSystem::get_worker_count() const { return workers.size(); }

    Worker& JobSystem::get_worker(const size_t index) const
    {
        assert(index < workers.size());
        std::lock_guard lock{ workers_mutex };
        return *workers[index];
    }

    void JobSystem::resize(const size_t new_size)
    {
        std::lock_guard lock{ workers_mutex };

        if (const size_t currentSize = workers.size();
            new_size > currentSize)
        {
            for (size_t i = currentSize; i < new_size; i++)
                workers.push_back(std::make_unique<Worker>(this, i));
        }
        else if (new_size < currentSize)
        {
            for (size_t i = new_size; i < currentSize; i++) workers[i]->stop();
            workers.resize(new_size);
        }
    }

    void JobSystem::wait()
    {
        std::unique_lock lock(workers_mutex);
        cv.wait(lock, [this] { return pending_jobs.load() == 0; });
    }
}
