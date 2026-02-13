#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragDir;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cameraUBO;

void main()
{
    float one = clamp(inNormal.x + inTexCoord.x + 1.0f, 0.0f, 1.0f); // to silence unused warnings

    fragDir = inPosition;

    mat4 viewRotation = mat4(mat3(cameraUBO.view));
    vec4 pos = cameraUBO.proj * viewRotation * vec4(inPosition, one);

    gl_Position = pos.xyww;
}