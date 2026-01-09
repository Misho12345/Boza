module boza.core;

import :time;

namespace boza
{
    void Time::init()
    {
        last_frame_time_ = std::chrono::steady_clock::now();
    }

    void Time::update()
    {
        const auto current_time = std::chrono::steady_clock::now();

        // calculate elapsed time since the last update() call
        const std::chrono::duration<float> duration = current_time - last_frame_time_;
        last_frame_time_ = current_time;

        // get the unscaled duration and add it to the total unscaled time
        unscaled_delta_time_ = duration.count();
        unscaled_time_       += unscaled_delta_time_;

        // get the scaled duration and add it to the total scaled time
        delta_time_ = unscaled_delta_time_ * time_scale;
        time_       += delta_time_;
    }
}