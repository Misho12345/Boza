#version 450

layout(push_constant) uniform PushConstant
{
    vec4 colors[3];
} pushConstant;

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(fragColor, 1.0);
}
