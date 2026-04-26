#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) flat in vec4 fragTint;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cameraUBO;

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

struct GpuPointLight {
    vec4 position_range;
    vec4 color_intensity;
    vec4 shadow_data;
};

struct GpuDirectionalLight {
    vec4 direction_shadow;
    vec4 color_intensity;
};

struct GpuSpotLight {
    vec4 position_range;
    vec4 direction_angles;
    vec4 color_intensity;
    vec4 shadow_data;
};

struct GpuClusterRecord {
    uvec4 point_spot_ranges;
};

layout(set = 0, binding = 7, std430) readonly buffer PointLightBuffer {
    GpuPointLight items[];
} pointLights;

layout(set = 0, binding = 8, std430) readonly buffer DirectionalLightBuffer {
    GpuDirectionalLight items[];
} directionalLights;

layout(set = 0, binding = 9, std430) readonly buffer SpotLightBuffer {
    GpuSpotLight items[];
} spotLights;

layout(set = 0, binding = 10, std430) readonly buffer ClusterRecordBuffer {
    GpuClusterRecord items[];
} clusterRecords;

layout(set = 0, binding = 11, std430) readonly buffer ClusterLightIndexBuffer {
    uint items[];
} clusterLightIndices;

layout(set = 0, binding = 13) uniform sampler2DArray directional_shadow_map;

layout(set = 0, binding = 14, std430) readonly buffer PointShadowMatrixBuffer {
    mat4 items[];
} pointShadowMatrices;

layout(set = 0, binding = 15) uniform sampler2DArray point_shadow_maps;

layout(set = 0, binding = 16, std430) readonly buffer SpotShadowMatrixBuffer {
    mat4 items[];
} spotShadowMatrices;

layout(set = 0, binding = 17) uniform sampler2DArray spot_shadow_maps;

const float PI = 3.14159265359;

float sample_shadow_map_2d(sampler2D shadow_map, vec2 uv, float receiver_depth, float strength)
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

    vec2 texel_size = 1.0 / vec2(textureSize(shadow_map, 0));
    float shadowed_value = 1.0 - clamp(strength, 0.0, 0.85);
    float filter_width = max(0.00008, texel_size.x * 1.1);
    float visibility = 0.0;

    for (int i = 0; i < 16; ++i) {
        vec2 offset = poisson_disk[i] * texel_size * 1.35;
        float map_depth = texture(shadow_map, uv + offset).r;
        float lit = smoothstep(receiver_depth - filter_width, receiver_depth + filter_width, map_depth);
        visibility += mix(shadowed_value, 1.0, lit);
    }

    return visibility / 16.0;
}

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
    float filter_width = max(0.0001, texel_size.x * 1.25);
    float visibility = 0.0;

    for (int i = 0; i < 16; ++i) {
        vec2 offset = poisson_disk[i] * texel_size * 1.45;
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
    float normal_offset = lightingUBO.shadow_params.x * (0.2 + 0.8 * (1.0 - normal_term));
    float depth_bias = lightingUBO.shadow_params.y * (1.0 + (1.0 - normal_term));

    vec4 light_clip = lightingUBO.directional_shadow_view_projections[cascade_index] *
        vec4(world_pos + normal * normal_offset, 1.0);

    if (abs(light_clip.w) <= 1e-5) return 1.0;

    vec3 shadow_coord = light_clip.xyz / light_clip.w;
    if (shadow_coord.x < -1.0 || shadow_coord.x > 1.0 ||
        shadow_coord.y < -1.0 || shadow_coord.y > 1.0 ||
        shadow_coord.z < 0.0 || shadow_coord.z > 1.0) {
        return 1.0;
    }

    vec2 uv = shadow_coord.xy * 0.5 + 0.5;
    return sample_shadow_map_array(directional_shadow_map, uv, float(cascade_index), shadow_coord.z - depth_bias, strength);
}

uint point_shadow_face(vec3 from_light)
{
    vec3 abs_dir = abs(from_light);

    if (abs_dir.x >= abs_dir.y && abs_dir.x >= abs_dir.z) {
        return from_light.x >= 0.0 ? 0u : 1u;
    }
    if (abs_dir.y >= abs_dir.x && abs_dir.y >= abs_dir.z) {
        return from_light.y >= 0.0 ? 2u : 3u;
    }

    return from_light.z >= 0.0 ? 4u : 5u;
}

float point_shadow_factor(GpuPointLight light, vec3 world_pos, vec3 normal, vec3 light_dir)
{
    if (light.shadow_data.x <= 0.5) return 1.0;

    vec3 from_light = world_pos - light.position_range.xyz;
    uint matrix_index = uint(light.shadow_data.z + 0.5) * 6u + point_shadow_face(from_light);
    vec4 light_clip = pointShadowMatrices.items[matrix_index] *
        vec4(world_pos + normal * (lightingUBO.shadow_params.x * 0.5), 1.0);

    if (abs(light_clip.w) <= 1e-5) return 1.0;

    vec3 shadow_coord = light_clip.xyz / light_clip.w;
    if (shadow_coord.x < -1.0 || shadow_coord.x > 1.0 ||
        shadow_coord.y < -1.0 || shadow_coord.y > 1.0 ||
        shadow_coord.z < 0.0 || shadow_coord.z > 1.0) {
        return 1.0;
    }

    float normal_term = clamp(dot(normal, light_dir), 0.0, 1.0);
    float depth_bias = max(0.00006, lightingUBO.shadow_params.y * 1.1) * (1.0 + (1.0 - normal_term));
    vec2 uv = shadow_coord.xy * 0.5 + 0.5;
    return sample_shadow_map_array(point_shadow_maps, uv, float(matrix_index), shadow_coord.z - depth_bias, light.shadow_data.y);
}

float spot_shadow_factor(GpuSpotLight light, vec3 world_pos, vec3 normal, vec3 light_dir)
{
    if (light.shadow_data.x <= 0.5) return 1.0;

    uint matrix_index = uint(light.shadow_data.w + 0.5);
    vec4 light_clip = spotShadowMatrices.items[matrix_index] *
        vec4(world_pos + normal * (lightingUBO.shadow_params.x * 0.5), 1.0);

    if (abs(light_clip.w) <= 1e-5) return 1.0;

    vec3 shadow_coord = light_clip.xyz / light_clip.w;
    if (shadow_coord.x < -1.0 || shadow_coord.x > 1.0 ||
        shadow_coord.y < -1.0 || shadow_coord.y > 1.0 ||
        shadow_coord.z < 0.0 || shadow_coord.z > 1.0) {
        return 1.0;
    }

    float normal_term = clamp(dot(normal, light_dir), 0.0, 1.0);
    float depth_bias = max(0.00005, lightingUBO.shadow_params.y) * (1.0 + 0.75 * (1.0 - normal_term));
    vec2 uv = shadow_coord.xy * 0.5 + 0.5;
    return sample_shadow_map_array(spot_shadow_maps, uv, float(matrix_index), shadow_coord.z - depth_bias, light.shadow_data.y);
}

float distribution_ggx(vec3 normal, vec3 half_dir, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float normal_dot_half = max(dot(normal, half_dir), 0.0);
    float denom = normal_dot_half * normal_dot_half * (alpha2 - 1.0) + 1.0;
    return alpha2 / max(PI * denom * denom, 1e-4);
}

float geometry_schlick_ggx(float normal_dot_direction, float roughness)
{
    float k = (roughness + 1.0);
    k = (k * k) / 8.0;
    return normal_dot_direction / max(normal_dot_direction * (1.0 - k) + k, 1e-4);
}

float geometry_smith(vec3 normal, vec3 view_dir, vec3 light_dir, float roughness)
{
    float normal_dot_view = max(dot(normal, view_dir), 0.0);
    float normal_dot_light = max(dot(normal, light_dir), 0.0);
    return geometry_schlick_ggx(normal_dot_view, roughness) * geometry_schlick_ggx(normal_dot_light, roughness);
}

vec3 fresnel_schlick(float cos_theta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 shade(vec3 albedo, vec3 normal, vec3 view_dir, vec3 light_dir, vec3 radiance)
{
    const float roughness = 0.55;
    const float metallic = 0.02;

    vec3 half_dir = normalize(view_dir + light_dir);
    float normal_dot_view = max(dot(normal, view_dir), 0.0);
    float normal_dot_light = max(dot(normal, light_dir), 0.0);
    float half_dot_view = max(dot(half_dir, view_dir), 0.0);

    if (normal_dot_view <= 0.0 || normal_dot_light <= 0.0) return vec3(0.0);

    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = fresnel_schlick(half_dot_view, f0);
    float distribution = distribution_ggx(normal, half_dir, roughness);
    float geometry = geometry_smith(normal, view_dir, light_dir, roughness);

    vec3 specular = (distribution * geometry * fresnel) / max(4.0 * normal_dot_view * normal_dot_light, 1e-4);
    vec3 kd = (vec3(1.0) - fresnel) * (1.0 - metallic);
    vec3 diffuse = kd * albedo / PI;

    return (diffuse + specular) * radiance * normal_dot_light;
}

uint resolve_cluster_index(vec3 world_pos)
{
    uvec3 cluster_grid = max(lightingUBO.cluster_grid.xyz, uvec3(1u));
    vec2 screen_size = max(lightingUBO.screen_params.xy, vec2(1.0));
    float near_clip = max(lightingUBO.screen_params.z, 0.001);
    float far_clip = max(lightingUBO.screen_params.w, near_clip + 1.0);

    vec4 view_pos = cameraUBO.view * vec4(world_pos, 1.0);
    vec4 clip_pos = cameraUBO.proj * view_pos;

    vec2 ndc = abs(clip_pos.w) > 1e-5 ? clip_pos.xy / clip_pos.w : vec2(0.0);
    vec2 screen_pos = (ndc * 0.5 + 0.5) * screen_size;
    float view_depth = clamp(abs(view_pos.z), near_clip, far_clip);
    float depth_ratio = max(view_depth / near_clip, 1.0);
    float far_ratio = max(far_clip / near_clip, 1.0001);
    float z_normalized = clamp(log2(depth_ratio) / log2(far_ratio), 0.0, 0.999999);

    uint cluster_x = uint(clamp(
        floor(screen_pos.x / screen_size.x * float(cluster_grid.x)),
        0.0,
        float(cluster_grid.x - 1u)));
    uint cluster_y = uint(clamp(
        floor(screen_pos.y / screen_size.y * float(cluster_grid.y)),
        0.0,
        float(cluster_grid.y - 1u)));
    uint cluster_z = uint(clamp(
        floor(z_normalized * float(cluster_grid.z)),
        0.0,
        float(cluster_grid.z - 1u)));

    return min(
        cluster_z * cluster_grid.x * cluster_grid.y + cluster_y * cluster_grid.x + cluster_x,
        max(lightingUBO.cluster_grid.w, 1u) - 1u);
}

void main()
{
    vec3 albedo = fragTint.rgb;
    vec3 normal = normalize(fragNormal);
    vec3 view_dir = normalize(lightingUBO.view_pos.xyz - fragPosWorld);
    vec3 result = albedo * lightingUBO.ambient_color.rgb * max(lightingUBO.ambient_color.w, 0.08);
    GpuClusterRecord cluster_record = clusterRecords.items[resolve_cluster_index(fragPosWorld)];

    for (uint i = 0u; i < lightingUBO.light_counts.y; ++i) {
        GpuDirectionalLight light = directionalLights.items[i];
        vec3 light_dir = normalize(-light.direction_shadow.xyz);
        float shadow = light.direction_shadow.w > 0.0
            ? directional_shadow_factor(fragPosWorld, normal, light_dir, light.direction_shadow.w)
            : 1.0;
        vec3 radiance = light.color_intensity.rgb * light.color_intensity.w * shadow;
        result += shade(albedo, normal, view_dir, light_dir, radiance);
    }

    for (uint i = 0u; i < cluster_record.point_spot_ranges.y; ++i) {
        uint light_index = clusterLightIndices.items[cluster_record.point_spot_ranges.x + i];
        if (light_index >= lightingUBO.light_counts.x) continue;

        GpuPointLight light = pointLights.items[light_index];
        vec3 to_light = light.position_range.xyz - fragPosWorld;
        float distance_to_light = length(to_light);
        if (distance_to_light <= 0.001 || distance_to_light > light.position_range.w) continue;

        vec3 light_dir = to_light / distance_to_light;
        float attenuation = 1.0 - clamp(distance_to_light / light.position_range.w, 0.0, 1.0);
        attenuation *= attenuation;
        float shadow = point_shadow_factor(light, fragPosWorld, normal, light_dir);
        vec3 radiance = light.color_intensity.rgb * light.color_intensity.w * attenuation * shadow;
        result += shade(albedo, normal, view_dir, light_dir, radiance);
    }

    for (uint i = 0u; i < cluster_record.point_spot_ranges.w; ++i) {
        uint light_index = clusterLightIndices.items[cluster_record.point_spot_ranges.z + i];
        if (light_index >= lightingUBO.light_counts.z) continue;

        GpuSpotLight light = spotLights.items[light_index];
        vec3 light_to_frag = fragPosWorld - light.position_range.xyz;
        float distance_to_light = length(light_to_frag);
        if (distance_to_light <= 0.001 || distance_to_light > light.position_range.w) continue;

        vec3 from_light = light_to_frag / distance_to_light;
        float spot_term = smoothstep(light.direction_angles.w, light.shadow_data.z, dot(from_light, normalize(light.direction_angles.xyz)));
        if (spot_term <= 0.0) continue;

        vec3 light_dir = -from_light;
        float attenuation = 1.0 - clamp(distance_to_light / light.position_range.w, 0.0, 1.0);
        attenuation *= attenuation;

        float shadow = spot_shadow_factor(light, fragPosWorld, normal, light_dir);
        vec3 radiance = light.color_intensity.rgb * light.color_intensity.w * attenuation * spot_term * shadow;
        result += shade(albedo, normal, view_dir, light_dir, radiance);
    }

    outColor = vec4(result, fragTint.a);
}
