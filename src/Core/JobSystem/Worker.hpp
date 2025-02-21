#pragma once
#include "boza_pch.hpp"
#include "Job.hpp"

namespace boza
{
    class JobSystem;

    class BOZA_API Worker
    {
    public:
        Worker(JobSystem* job_system, size_t id);
        ~Worker();

        void run();

        void push_job(const std::shared_ptr<Job>& job);
        std::shared_ptr<Job> pop_job();
        std::shared_ptr<Job> steal_job();

        void stop();

    private:
        JobSystem* job_system;
        size_t     id;

        std::deque<std::shared_ptr<Job>> local_queue;

        mutable std::mutex queue_mutex;
        std::atomic_bool   stop_flag{ false };
        std::thread        worker_thread;
    };
}
