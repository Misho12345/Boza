#version 450

layout(push_constant) uniform PushConstant
{
    vec4 colors[3];
} pushConstant;

layout(set = 0, binding = 0) uniform Positions
{
    vec4 positions[3];
} positions;

layout (location = 0) out vec3 fragColor;

void main()
{
    gl_Position = vec4(positions.positions[gl_VertexIndex].xy, 0.0, 1.0);
    fragColor = pushConstant.colors[gl_VertexIndex].xyz;
}
