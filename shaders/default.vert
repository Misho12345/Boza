#version 450

layout (set = 0, binding = 0) uniform UBO1 {
    vec2 offset;
} ubo1;

layout (set = 0, binding = 1) uniform UBO2 {
    vec2 scale;
} ubo2;

layout(push_constant) uniform PushConstants
{
    float rotationAngle;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
    mat2 rotationMatrix = mat2(
        cos(pushConstants.rotationAngle), sin(pushConstants.rotationAngle),
        -sin(pushConstants.rotationAngle), cos(pushConstants.rotationAngle));

    gl_Position = vec4((rotationMatrix * inPosition.xy + ubo1.offset) * ubo2.scale, inPosition.z, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;
}