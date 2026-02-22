export module material_showcase:components;

import std;
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

export struct Rotator
{
    float     rotation_speed{ 1.0f };
    glm::vec3 rotation_axis{ 0.0f, 1.0f, 0.0f };
};


export struct Orbiter
{
    glm::vec3 axis{ 0.0f, 1.0f, 0.0f };
    float     distance_from_target{ 5.0f };
    float     orbit_speed{ 50.0f };
    float     angle{ 0.0f };
};
