#version 450

layout(location = 0) in vec3 fragDir;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform samplerCube skybox_map;

void main()
{
    outColor = vec4(texture(skybox_map, normalize(fragDir)).xyz, 1.0);
}
