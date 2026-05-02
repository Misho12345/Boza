#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosWorld;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cameraUBO;

layout(set = 0, binding = 1) uniform sampler2D albedo_map;

layout(set = 0, binding = 2) uniform MaterialUBO {
    vec4 albedo_color;
    vec4 properties;
    vec4 detail;
} material;

layout(set = 0, binding = 3) uniform LightingUBO {
    vec4 view_pos;
    vec4 ambient_color;
    uvec4 light_counts;
    uvec4 cluster_grid;
    vec4 screen_params;
    vec4 shadow_params;
    vec4 directional_shadow_splits;
    mat4 directional_shadow_view_projections[4];
} lightingUBO;

struct GpuDirectionalLight {
    vec4 direction_shadow;
    vec4 color_intensity;
};

layout(set = 0, binding = 8, std430) readonly buffer DirectionalLightBuffer {
    GpuDirectionalLight items[];
} directionalLights;

layout(set = 0, binding = 13) uniform sampler2DArray directional_shadow_map;

float sample_shadow_map_array(sampler2DArray shadow_map, vec2 uv, float layer, float receiver_depth, float strength)
{
    const vec2 poisson_disk[16] = vec2[](
        vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
        vec2(-0.09418410, -0.92938870), vec2(0.34495938, 0.29387760),
        vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
        vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
        vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
        vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
        vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
        vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
    );

    vec2 texel_size = 1.0 / vec2(textureSize(shadow_map, 0).xy);
    float shadowed_value = 1.0 - clamp(strength, 0.0, 0.85);
    float filter_width = max(0.00012, texel_size.x * 1.6);
    float visibility = 0.0;

    for (int i = 0; i < 16; ++i)
    {
        vec2 offset = poisson_disk[i] * texel_size * 1.5;
        float map_depth = texture(shadow_map, vec3(uv + offset, layer)).r;
        float lit = smoothstep(receiver_depth - filter_width, receiver_depth + filter_width, map_depth);
        visibility += mix(shadowed_value, 1.0, lit);
    }

    return visibility / 16.0;
}

float directional_shadow_factor(vec3 world_pos, vec3 normal, vec3 light_dir, float strength)
{
    if (lightingUBO.shadow_params.w <= 0.0 || strength <= 0.0) return 1.0;

    float view_depth = abs((cameraUBO.view * vec4(world_pos, 1.0)).z);
    int cascade_index = 0;
    if (view_depth > lightingUBO.directional_shadow_splits.x) cascade_index = 1;
    if (view_depth > lightingUBO.directional_shadow_splits.y) cascade_index = 2;
    if (view_depth > lightingUBO.directional_shadow_splits.z) cascade_index = 3;

    float normal_term = clamp(dot(normal, light_dir), 0.0, 1.0);
    float normal_offset = lightingUBO.shadow_params.x * (0.35 + 0.65 * (1.0 - normal_term));
    float depth_bias = lightingUBO.shadow_params.y * (1.1 + (1.0 - normal_term));

    vec4 shadow_world_pos = vec4(world_pos + normal * normal_offset, 1.0);
    float visibility = 1.0;
    bool found_valid_cascade = false;

    for (int idx = 0; idx < 4; ++idx)
    {
        vec4 light_clip = lightingUBO.directional_shadow_view_projections[idx] * shadow_world_pos;
        if (abs(light_clip.w) <= 1e-5) continue;

        vec3 shadow_coord = light_clip.xyz / light_clip.w;
        if (shadow_coord.x < -1.0 || shadow_coord.x > 1.0 ||
            shadow_coord.y < -1.0 || shadow_coord.y > 1.0 ||
            shadow_coord.z < 0.0 || shadow_coord.z > 1.0)
        {
            continue;
        }

        vec2 uv = shadow_coord.xy * 0.5 + 0.5;
        float cascade_visibility = sample_shadow_map_array(
            directional_shadow_map,
            uv,
            float(idx),
            shadow_coord.z - depth_bias,
            strength);

        if (!found_valid_cascade)
        {
            visibility = cascade_visibility;
            found_valid_cascade = true;
        }
        else
        {
            visibility = min(visibility, cascade_visibility);
        }
    }

    return found_valid_cascade ? visibility : 1.0;
}

void main()
{
    vec4 tex_color = texture(albedo_map, fragTexCoord);
    vec3 albedo = tex_color.rgb * material.albedo_color.rgb;

    vec3 normal = normalize(fragNormal);
    if (!gl_FrontFacing) normal = -normal;

    vec3 ambient = albedo * lightingUBO.ambient_color.rgb * lightingUBO.ambient_color.w;
    vec3 lighting = ambient;

    for (uint i = 0u; i < lightingUBO.light_counts.y; ++i)
    {
        GpuDirectionalLight light = directionalLights.items[i];
        vec3 light_dir = normalize(-light.direction_shadow.xyz);
        float diffuse = max(dot(normal, light_dir), 0.0);
        if (diffuse <= 0.0) continue;

        float shadow = light.direction_shadow.w > 0.0
            ? directional_shadow_factor(fragPosWorld, normal, light_dir, light.direction_shadow.w)
            : 1.0;

        vec3 radiance = light.color_intensity.rgb * light.color_intensity.w;
        lighting += albedo * radiance * diffuse * shadow;
    }

    outColor = vec4(lighting, material.albedo_color.w * tex_color.a);
}
