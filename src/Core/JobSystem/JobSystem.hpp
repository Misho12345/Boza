#pragma once
#include "../../pch.hpp"
#include "Worker.hpp"

namespace boza
{
    class BOZA_API JobSystem
    {
    public:
        explicit JobSystem(size_t num_threads = std::thread::hardware_concurrency());
        ~JobSystem();

        template<typename F, typename... Args>
        auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

        template<typename F, typename... Args>
        auto submit(
            const std::vector<std::shared_future<void>>& dependencies,
            F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

        void enqueue_job(const std::shared_ptr<Job>& job);

        size_t  get_worker_count() const;
        Worker& get_worker(size_t index) const;

        void resize(size_t new_size);
        void wait();

        int get_pending_job_count() const { return pending_jobs.load(); }

    private:
        friend Worker;

        std::vector<std::unique_ptr<Worker>> workers;
        std::atomic_bool is_shutting_down{ false };

        mutable std::mutex      workers_mutex;
        std::condition_variable cv;
        std::atomic<int>        pending_jobs{ 0 };
    };
}

#include "JobSystem.inl"
