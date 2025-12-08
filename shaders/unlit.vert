#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cam;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {
    gl_Position = cam.proj * cam.view * pc.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragNormal = inNormal;
}

