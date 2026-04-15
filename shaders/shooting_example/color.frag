#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) flat in vec4 fragTint;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 3) uniform LightUBO {
    vec4 light_position;
    vec4 light_color;
    vec4 viewPos;
} lightUBO;

void main()
{
    vec3 albedo = fragTint.rgb;
    vec3 normal = normalize(fragNormal);
    vec3 light_dir = normalize(lightUBO.light_position.xyz - fragPosWorld);
    vec3 view_dir = normalize(lightUBO.viewPos.xyz - fragPosWorld);
    vec3 half_dir = normalize(light_dir + view_dir);

    vec3 light_color = lightUBO.light_color.xyz * lightUBO.light_color.w;

    float ambient_strength = 0.22;
    vec3 ambient = ambient_strength * light_color;

    float diffuse_term = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diffuse_term * light_color;

    float specular_term = pow(max(dot(normal, half_dir), 0.0), 18.0);
    vec3 specular = specular_term * 0.08 * light_color;

    vec3 color = (ambient + diffuse) * albedo + specular;
    outColor = vec4(color, fragTint.a);
}
