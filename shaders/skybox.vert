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
    fragDir = inPosition + inNormal * 0.0 + vec3(inTexCoord, 0.0) * 0.0;

    mat4 viewRotation = mat4(mat3(cameraUBO.view));
    vec4 pos = cameraUBO.proj * viewRotation * vec4(inPosition, 1.0);

    gl_Position = pos.xyww;
}
