module game.player:math;

import std;
import boza.common;

namespace game::player
{
    [[nodiscard]]
    inline float exp_alpha(const float delta_time, const float smooth_time)
    {
        if (smooth_time <= 0.0f) return 1.0f;
        return 1.0f - std::exp(-delta_time / smooth_time);
    }

    [[nodiscard]]
    inline float exp_smooth_scalar(
        const float current,
        const float target,
        const float delta_time,
        const float smooth_time) { return current + (target - current) * exp_alpha(delta_time, smooth_time); }

    [[nodiscard]]
    inline glm::vec3 exp_smooth_vec3(
        const glm::vec3& current,
        const glm::vec3& target,
        const float      delta_time,
        const float      smooth_time) { return current + (target - current) * exp_alpha(delta_time, smooth_time); }

    [[nodiscard]]
    inline float wrap_pi(float angle)
    {
        angle = std::fmod(angle + glm::pi<float>(), glm::two_pi<float>());
        if (angle < 0.0f) angle += glm::two_pi<float>();
        return angle - glm::pi<float>();
    }

    [[nodiscard]]
    inline float exp_smooth_angle(
        const float current,
        const float target,
        const float delta_time,
        const float smooth_time) { return current + wrap_pi(target - current) * exp_alpha(delta_time, smooth_time); }
}
