#include "PhysicsSystem.hpp"

#include "Logger.hpp"
#include "Core/GameObject.hpp"
#include "Core/JobSystem/JobSystem.hpp"

namespace boza
{
    STATIC_VARIABLE_FN(PhysicsSystem::thread, {})
    STATIC_VARIABLE_FN(PhysicsSystem::stop_flag, { false })

    void PhysicsSystem::start()
    {
        thread() = std::thread{ run };
    }

    void PhysicsSystem::stop()
    {
        stop_flag() = true;
        if (thread().joinable()) thread().join();
    }


    void PhysicsSystem::run()
    {
        time_point last_time = clock::now();
        std::vector<JobSystem::task_id> tasks(16);

        while (!stop_flag().load())
        {
            time_point current_time = clock::now();
            accumulated_time += std::chrono::duration_cast<duration>(current_time - last_time);
            last_time = current_time;

            if (accumulated_time > max_catch_up_time)
            {
                int dropped_steps = (accumulated_time - max_catch_up_time) / fixed_delta_time;
                Logger::warn("Physics system is running behind! Dropping {} steps.", dropped_steps);
                accumulated_time = max_catch_up_time;
            }

            while (accumulated_time >= fixed_delta_time)
            {
                for (const auto* object : Scene::get_active_scene().get_game_objects())
                {
                    for (auto* behaviour : object->behaviours)
                        tasks.push_back(JobSystem::push_task([behaviour] { behaviour->fixed_update(); }));
                }

                for (const auto& task : tasks)
                    JobSystem::wait_for_task(task);
                tasks.clear();

                accumulated_time -= fixed_delta_time;
            }

            if (time_point end_time = last_time + fixed_delta_time;
                clock::now() < end_time)
                std::this_thread::sleep_until(end_time);
        }
    }
}
