#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D albedo_map;

layout(set = 0, binding = 2) uniform MaterialUBO {
    vec4 albedo_color;
    vec4 properties;
} material;

void main() {
    vec4 tex_color = texture(albedo_map, fragTexCoord);
    outColor = tex_color * material.albedo_color;
}

