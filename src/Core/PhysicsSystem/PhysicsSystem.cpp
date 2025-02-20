#include "PhysicsSystem.hpp"

#include "Logger.hpp"
#include "../GameObject.hpp"

namespace boza
{
    void PhysicsSystem::start() { physics_thread = std::thread(&PhysicsSystem::run, this); }
    void PhysicsSystem::stop()
    {
        stop_flag = true;
        if (physics_thread.joinable()) physics_thread.join();
    }


    void PhysicsSystem::run()
    {
        time_point last_time = clock::now();

        while (!stop_flag)
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
                        job_system.submit([behaviour] { behaviour->fixed_update(); });
                }

                job_system.wait();
                accumulated_time -= fixed_delta_time;
            }

            if (time_point end_time = last_time + fixed_delta_time;
                clock::now() < end_time)
                std::this_thread::sleep_until(end_time);
        }
    }
}
