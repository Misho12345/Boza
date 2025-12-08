#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosWorld;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D albedo_map;

layout(set = 0, binding = 2) uniform MaterialUBO {
    vec4 albedo_color;
    vec4 properties;
} material;

layout(set = 0, binding = 3) uniform LightUBO {
    vec4 light_position;
    vec4 light_color;
    vec4 viewPos;
} lightUBO;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightUBO.light_position.xyz - fragPosWorld);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightUBO.light_color.rgb * lightUBO.light_color.a;

    vec3 ambient = vec3(0.15);

    vec4 tex_color = texture(albedo_map, fragTexCoord);
    vec3 result = (ambient + diffuse) * tex_color.rgb * material.albedo_color.rgb;

    outColor = vec4(result, tex_color.a * material.albedo_color.a);
}

