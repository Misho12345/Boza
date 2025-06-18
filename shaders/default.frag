#version 450

layout(set = 0, binding = 2) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(fragColor * texture(textureSampler, fragTexCoord).rgb, 1.0);
}