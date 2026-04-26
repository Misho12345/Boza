export module clustered_lights:components;

import boza;

using namespace boza;

export struct CameraController
{
    float move_speed{ 45.0f };
    float sensitivity{ 0.0015f };
    float yaw{ 0.0f };
    float pitch{ 0.0f };
};

export struct Rotator
{
    glm::vec3 axis{ 0.0f, 1.0f, 0.0f };
    float speed{ 0.12f };
};
