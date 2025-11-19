module;

#include "api.hpp"

export module boza.core:time;
import std;

export namespace boza
{
    class BOZA_API Time
    {
    public:
        static void init();
        static void update();

        static float fixed_delta_time() { return fixed_delta_time_; }
        static void  set_fixed_delta_time(const float dt) { fixed_delta_time_ = dt; }

        static float delta_time() { return delta_time_; }
        static float unscaled_delta_time() { return unscaled_delta_time_; }

        static float time_scale() { return time_scale_; }
        static void  set_time_scale(const float scale) { time_scale_ = scale; }

        static float time() { return time_; }
        static float unscaled_time() { return unscaled_time_; }

        static std::uint64_t frame_count() { return frame_count_; }

        static float fps() { return fps_; }

    private:
        struct TimeData;
        static std::unique_ptr<TimeData> time_data_;

        static inline float fixed_delta_time_ = 1.0f / 60.0f;
        static inline float delta_time_       = 0.0f;

        static inline float unscaled_delta_time_ = 0.0f;
        static inline float time_scale_          = 1.0f;

        static inline float         time_          = 0.0f;
        static inline float         unscaled_time_ = 0.0f;
        static inline std::uint64_t frame_count_   = 0;
        static inline float         fps_           = 0.0f;
    };
}
