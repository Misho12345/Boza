export module instancing_example:components;

import boza;

export struct CameraController
{
    float move_speed{ 1.0f };
    float sensitivity{ 0.001f };

    float move_accel_smooth_time{ 0.08f };
    float move_decel_smooth_time{ 0.03f };
    float rotation_smooth_time{ 0.001f };

    glm::vec3 current_velocity{ 0.0f };

    float yaw{ 0.0f };
    float pitch{ 0.0f };
    float target_yaw{ 0.0f };
    float target_pitch{ 0.0f };
};