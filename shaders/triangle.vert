#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    float time;
} pc;

void main() {
    float angle = pc.time;
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);

    mat2 rotation = mat2(
        cosAngle, -sinAngle,
        sinAngle, cosAngle
    );

    vec2 rotatedPos = rotation * inPosition;
    gl_Position = vec4(rotatedPos, 0.0, 1.0);
    fragColor = inColor;
}
