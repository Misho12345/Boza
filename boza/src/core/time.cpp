module boza.core;

import :time;
import std;

namespace boza
{
    void Time::init()
    {
        last_frame_time_ = std::chrono::steady_clock::now();

        time_ = 0.0f;
        delta_time_ = 0.0f;

        unscaled_time_ = 0.0f;
        unscaled_delta_time_ = 0.0f;
        frame_count_ = 0;
    }

    void Time::update()
    {
        const auto current_time = std::chrono::steady_clock::now();
        const std::chrono::duration<float> duration = current_time - last_frame_time_;
        last_frame_time_ = current_time;

        unscaled_delta_time_ = duration.count();
        unscaled_time_ += unscaled_delta_time_;

        delta_time_ = unscaled_delta_time_ * time_scale;
        time_ += delta_time_;

        ++frame_count_;
    }
}
