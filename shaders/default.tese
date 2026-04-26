#version 450

layout(triangles, fractional_odd_spacing, ccw) in;

layout(location = 0) in vec3 tcNormal[];
layout(location = 1) in vec2 tcTexCoord[];
layout(location = 2) in vec3 tcWorldPos[];

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosWorld;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cameraUBO;

layout(set = 0, binding = 2) uniform MaterialUBO {
    vec4 albedo_color;
    vec4 properties;
    vec4 detail;
} material;

layout(set = 0, binding = 22) uniform sampler2D height_map;

layout(push_constant) uniform PushConstants {
    mat4 shadow_view_projection;
    mat4 model;
    uint use_instancing;
    uint render_mode;
} pc;

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

vec3 triplanar_weights(vec3 normal)
{
    vec3 weights = pow(abs(normal), vec3(4.0));
    return weights / max(dot(weights, vec3(1.0)), 1e-4);
}

vec2 projected_uv_x(vec3 world_pos, vec3 normal, vec2 uv_scale)
{
    return vec2(world_pos.z, world_pos.y) * uv_scale * vec2(sign(normal.x == 0.0 ? 1.0 : normal.x), 1.0);
}

vec2 projected_uv_y(vec3 world_pos, vec3 normal, vec2 uv_scale)
{
    return vec2(world_pos.x, world_pos.z) * uv_scale * vec2(sign(normal.y == 0.0 ? 1.0 : normal.y), 1.0);
}

vec2 projected_uv_z(vec3 world_pos, vec3 normal, vec2 uv_scale)
{
    return vec2(world_pos.x, world_pos.y) * uv_scale * vec2(-sign(normal.z == 0.0 ? 1.0 : normal.z), 1.0);
}

vec2 box_uv(vec3 world_pos, vec3 normal, vec2 uv_scale)
{
    vec3 abs_normal = abs(normal);

    if (abs_normal.x >= abs_normal.y && abs_normal.x >= abs_normal.z)
        return projected_uv_x(world_pos, normal, uv_scale);

    if (abs_normal.y >= abs_normal.z)
        return projected_uv_y(world_pos, normal, uv_scale);

    return projected_uv_z(world_pos, normal, uv_scale);
}

float sample_height(vec3 world_pos, vec3 normal, vec2 uv, vec2 uv_scale)
{
    if (material.detail.z > 0.5)
    {
        return texture(height_map, world_pos.xz * triplanar_density(uv_scale)).r;
    }

    int mapping_mode = surface_mapping_mode();

    if (mapping_mode == SurfaceMappingTriplanar)
    {
        vec3 weights = triplanar_weights(normal);
        vec2 density = vec2(triplanar_density(uv_scale));
        vec2 uv_x = projected_uv_x(world_pos, normal, density);
        vec2 uv_y = projected_uv_y(world_pos, normal, density);
        vec2 uv_z = projected_uv_z(world_pos, normal, density);

        float x_sample = texture(height_map, uv_x).r;
        float y_sample = texture(height_map, uv_y).r;
        float z_sample = texture(height_map, uv_z).r;
        return x_sample * weights.x + y_sample * weights.y + z_sample * weights.z;
    }

    if (mapping_mode == SurfaceMappingWorldBox)
    {
        return texture(height_map, box_uv(world_pos, normal, uv_scale)).r;
    }

    return texture(height_map, uv * uv_scale).r;
}

void main()
{
    vec3 bary = gl_TessCoord;

    vec3 world_pos =
        tcWorldPos[0] * bary.x +
        tcWorldPos[1] * bary.y +
        tcWorldPos[2] * bary.z;

    vec3 normal = normalize(
        tcNormal[0] * bary.x +
        tcNormal[1] * bary.y +
        tcNormal[2] * bary.z);

    vec2 uv =
        tcTexCoord[0] * bary.x +
        tcTexCoord[1] * bary.y +
        tcTexCoord[2] * bary.z;

    vec2 uv_scale = resolve_uv_scale();
    float displacement_scale = material.detail.x;
    vec3 view_space_pos = (cameraUBO.view * vec4(world_pos, 1.0)).xyz;
    displacement_scale *= displacement_distance_fade(length(view_space_pos));

    if (displacement_scale <= 0.0001)
    {
        fragPosWorld = world_pos;
        fragNormal = normal;
        fragTexCoord = uv;

        if (pc.render_mode != 0u)
            gl_Position = pc.shadow_view_projection * vec4(world_pos, 1.0);
        else
            gl_Position = cameraUBO.proj * cameraUBO.view * vec4(world_pos, 1.0);
        return;
    }

    float height = sample_height(world_pos, normal, uv, uv_scale);
    vec3 displacement_direction = material.detail.z > 0.5 ? vec3(0.0, 1.0, 0.0) : normal;
    world_pos += displacement_direction * ((height - 0.5) * 2.0 * displacement_scale);

    fragPosWorld = world_pos;
    fragNormal = normal;
    fragTexCoord = uv;

    if (pc.render_mode != 0u)
        gl_Position = pc.shadow_view_projection * vec4(world_pos, 1.0);
    else
        gl_Position = cameraUBO.proj * cameraUBO.view * vec4(world_pos, 1.0);
}
