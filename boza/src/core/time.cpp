module boza.core;

namespace boza
{
    struct Time::TimeData
    {
        std::chrono::steady_clock::time_point last_frame_time;
        std::chrono::steady_clock::time_point start_time;
    };

    std::unique_ptr<Time::TimeData> Time::time_data_ = std::make_unique<TimeData>();

    void Time::init()
    {
        time_data_->start_time      = std::chrono::steady_clock::now();
        time_data_->last_frame_time = time_data_->start_time;

        frame_count_   = 0;
        time_          = 0.0f;
        unscaled_time_ = 0.0f;
    }

    void Time::update()
    {
        const auto current_time = std::chrono::steady_clock::now();

        const std::chrono::duration<float> duration = current_time - time_data_->last_frame_time;

        unscaled_delta_time_ = duration.count();
        delta_time_          = unscaled_delta_time_ * time_scale_;

        time_data_->last_frame_time = current_time;

        unscaled_time_ += unscaled_delta_time_;
        time_ += delta_time_;

        ++frame_count_;

        if (unscaled_delta_time_ > 0.0f) fps_ = 1.0f / unscaled_delta_time_;
    }
}