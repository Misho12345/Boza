#version 450

layout(vertices = 3) out;

layout(location = 0) in vec3 inNormal[];
layout(location = 1) in vec2 inTexCoord[];
layout(location = 2) in vec3 inWorldPos[];

layout(location = 0) out vec3 tcNormal[];
layout(location = 1) out vec2 tcTexCoord[];
layout(location = 2) out vec3 tcWorldPos[];

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

layout(set = 0, binding = 2) uniform MaterialUBO {
    vec4 albedo_color;
    vec4 properties;
    vec4 detail;
} material;

const int SurfaceMappingUv = 0;
const int SurfaceMappingTriplanar = 1;
const int SurfaceMappingWorldBox = 2;

int surface_mapping_mode()
{
    return int(clamp(floor(material.detail.y + 0.5), 0.0, 2.0));
}

vec2 resolve_uv_scale()
{
    float u_scale = abs(material.properties.z) > 1e-4 ? material.properties.z : 1.0;
    float v_scale = abs(material.properties.w) > 1e-4 ? material.properties.w : u_scale;
    return max(vec2(u_scale, v_scale), vec2(1e-4));
}

float triplanar_density(vec2 uv_scale)
{
    return max((uv_scale.x + uv_scale.y) * 0.5, 1e-4);
}

float displacement_distance_fade(float distance_to_camera)
{
    bool terrain_like = material.detail.z > 0.5;

    float full_distance = terrain_like ? 24.0 : 10.0;
    float fade_distance = terrain_like ? 52.0 : 22.0;

    return 1.0 - smoothstep(full_distance, fade_distance, distance_to_camera);
}

float edge_detail_frequency(vec3 a, vec3 b, vec2 uv_a, vec2 uv_b)
{
    vec2 uv_scale = resolve_uv_scale();
    int mapping_mode = surface_mapping_mode();

    if (mapping_mode == SurfaceMappingUv)
        return length((uv_b - uv_a) * uv_scale);

    float world_density = mapping_mode == SurfaceMappingTriplanar
        ? triplanar_density(uv_scale)
        : max(uv_scale.x, uv_scale.y);

    return length(b - a) * world_density;
}

float tessellation_level_for_edge(vec3 a, vec3 b, vec2 uv_a, vec2 uv_b)
{
    float displacement_scale = max(material.detail.x, 0.0);
    if (displacement_scale <= 0.0001) return 1.0;

    float tessellation_scale = max(material.detail.w, 0.25);

    float detail_bias = clamp(displacement_scale * 10.0, 0.0, 1.0);
    vec3 edge_midpoint = (a + b) * 0.5;
    float distance_to_camera = distance(lightingUBO.view_pos.xyz, edge_midpoint);
    float distance_fade = displacement_distance_fade(distance_to_camera);
    if (distance_fade <= 0.0001) return 1.0;

    float edge_length = max(length(b - a), 1e-4);
    float detail_frequency = edge_detail_frequency(a, b, uv_a, uv_b);

    float size_multiplier = 1.0 + clamp(edge_length * 0.08, 0.0, 3.0);
    float detail_multiplier = 1.0 + clamp(detail_frequency * 0.4, 0.0, 4.0);
    float distance_falloff = clamp(distance_to_camera / (36.0 + edge_length * 1.5), 0.0, 1.0);

    detail_bias *= mix(0.55, 1.0, distance_fade);

    float near_level = mix(14.0, 28.0, detail_bias) * size_multiplier * detail_multiplier * tessellation_scale;
    float far_level = mix(4.0, 8.0, detail_bias) * min(detail_multiplier, 2.5) * sqrt(tessellation_scale);

    return clamp(mix(near_level, far_level, distance_falloff), 1.0, 96.0);
}

void main()
{
    tcNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    tcTexCoord[gl_InvocationID] = inTexCoord[gl_InvocationID];
    tcWorldPos[gl_InvocationID] = inWorldPos[gl_InvocationID];
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    barrier();

    if (gl_InvocationID != 0) return;

    gl_TessLevelOuter[0] = tessellation_level_for_edge(tcWorldPos[1], tcWorldPos[2], tcTexCoord[1], tcTexCoord[2]);
    gl_TessLevelOuter[1] = tessellation_level_for_edge(tcWorldPos[2], tcWorldPos[0], tcTexCoord[2], tcTexCoord[0]);
    gl_TessLevelOuter[2] = tessellation_level_for_edge(tcWorldPos[0], tcWorldPos[1], tcTexCoord[0], tcTexCoord[1]);
    gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[1] + gl_TessLevelOuter[2]) / 3.0;
}
