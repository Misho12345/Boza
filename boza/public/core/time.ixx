module;

#include "api.hpp"

export module boza.core:time;

import std;
import boza.common;

namespace boza::app { class GameLoop; }

export namespace boza
{
    /**
     * @brief Global engine time utility.
     *
     * Provides access to frame-based timing information such as
     * total elapsed time and per-frame delta time.
     *
     * All values are updated once per frame by the engine's main loop.
     * This class is static-only and intended to be read-only for users.
     */
    class BOZA_API Time final
    {
        /// Returns the total scaled time since engine start (seconds).
        static float get_time() { return time_; }

        /// Returns the scaled delta time of the last frame (seconds).
        static float get_delta_time() { return delta_time_; }

    public:
        Time() = delete;

        /**
         * @brief Total scaled time since engine start (seconds).
         *
         * Read-only property updated once per frame.
         */
        static inline GlobalProperty<&Time::get_time> time;

        /**
         * @brief Scaled duration of the last frame (seconds).
         *
         * Read-only property typically used for frame-dependent updates.
         */
        static inline GlobalProperty<&Time::get_delta_time> delta_time;

        /**
         * @brief Fixed time step used for fixed-rate updates (seconds).
         *
         * Commonly used for physics or deterministic simulation loops.
         * This value is user-configurable and not updated automatically.
         */
        static inline float fixed_delta_time{};

        /**
         * @brief Time scale multiplier applied to delta time.
         *
         * A value of 1.0 represents real-time.
         * Values less than 1.0 slow down time, greater than 1.0 speed it up.
         */
        static inline float time_scale{ 1.0f };

    private:
        /**
         * @brief Initializes the time system.
         *
         * Called once during engine startup by GameLoop.
         */
        static void init();

        /**
         * @brief Updates all time values for the current frame.
         *
         * Called once per frame by GameLoop.
         */
        static void update();

        /// Timestamp of the previous frame.
        static inline std::chrono::steady_clock::time_point last_frame_time_;

        /// Accumulated scaled time since engine start.
        static inline float time_{ 0.0f };

        /// Scaled delta time of the last frame.
        static inline float delta_time_{ 0.0f };

        /// Accumulated unscaled time since engine start.
        static inline float unscaled_time_{ 0.0f };

        /// Unscaled delta time of the last frame.
        static inline float unscaled_delta_time_{ 0.0f };

        friend class app::GameLoop;
        template<auto...> friend class GlobalProperty;
    };
}
