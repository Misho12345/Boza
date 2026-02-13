module;

#include "api.hpp"

export module boza.core:time;

import std;

namespace boza::app { class GameLoop; }

export namespace boza
{
    class BOZA_API Time final
    {
    public:
        Time() = delete;

        static float time() { return time_; }
        static float delta_time() { return delta_time_; }
        static float unscaled_time() { return unscaled_time_; }
        static float unscaled_delta_time() { return unscaled_delta_time_; }
        static std::uint64_t frame_count() { return frame_count_; }

        static inline float fixed_delta_time{ 0.0f };
        static inline float time_scale{ 1.0f };

    private:
        static void init();
        static void update();

        static inline std::chrono::steady_clock::time_point last_frame_time_{};

        static inline float time_{ 0.0f };
        static inline float delta_time_{ 0.0f };

        static inline float unscaled_time_{ 0.0f };
        static inline float unscaled_delta_time_{ 0.0f };
        static inline std::uint64_t frame_count_{ 0 };

        friend class app::GameLoop;
    };
}
