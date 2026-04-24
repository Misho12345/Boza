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
    vec4 texColor = texture(albedo_map, fragTexCoord);
    vec3 albedo = texColor.rgb * material.albedo_color.rgb;

    float lightIntensity = lightUBO.light_color.w;
    vec3 lightColor = lightUBO.light_color.xyz * lightIntensity;

    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightUBO.light_position.xyz - fragPosWorld);
    vec3 viewDir = normalize(lightUBO.viewPos.xyz - fragPosWorld);
    vec3 halfDir = normalize(lightDir + viewDir);

    // Ambient
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float roughness = clamp(material.properties.y, 0.04, 1.0);
    float specularStrength = material.properties.z * 0.2;
    float shininess = mix(24.0, 4.0, roughness);
    float spec = pow(max(dot(norm, halfDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse) * albedo + specular;
    outColor = vec4(result, material.albedo_color.w * texColor.a);
}
