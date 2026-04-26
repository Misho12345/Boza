#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(push_constant) uniform ShadowPushConstants {
    mat4 light_view_projection;
    mat4 model;
} pc;

void main()
{
    vec3 layout_keep_alive = inNormal * 1e-8 + vec3(inTexCoord, 0.0) * 1e-8;
    gl_Position = pc.light_view_projection * pc.model * vec4(inPosition + layout_keep_alive, 1.0);
}
