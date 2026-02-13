#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosWorld;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D albedo_map;

layout(set = 0, binding = 2) uniform MaterialUBO {
    vec4 albedo_color;   // xyz = color, w = alpha
    vec4 properties;     // x = metallic, y = roughness, z = specular, w = unused
} material;

layout(set = 0, binding = 3) uniform LightUBO {
    vec4 light_position;
    vec4 light_color;     // xyz = color, w = intensity
    vec4 viewPos;
} lightUBO;

void main() {
    // Sample albedo texture and combine with material color
    vec4 texColor = texture(albedo_map, fragTexCoord);
    vec3 albedo = texColor.rgb * material.albedo_color.rgb;

    // Extract light intensity from the w component
    float lightIntensity = lightUBO.light_color.w;
    vec3 lightColor = lightUBO.light_color.xyz * lightIntensity;

    // Ambient
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightUBO.light_position.xyz - fragPosWorld);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (using material properties)
    vec3 viewDir = normalize(lightUBO.viewPos.xyz - fragPosWorld);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0 * (1.0 - material.properties.y));
    vec3 specular = material.properties.z * spec * lightColor;

    // Combine lighting
    vec3 result = (ambient + diffuse + specular) * albedo;

    outColor = vec4(result, material.albedo_color.w * texColor.a);
}