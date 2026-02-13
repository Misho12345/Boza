#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragPosWorld;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cameraUBO;

layout(set = 0, binding = 4) uniform TimeUBO {
    float time;
    float delta_time;
    vec2 padding;
} timeUBO;

struct GrassData {
    mat4 model;
};

layout(set = 0, binding = 5) readonly buffer PcInstances {
    GrassData blades[];
} pc_instances;

layout(push_constant) uniform PushConstants {
    GrassData pc_data;
    uint use_instancing;
} pc;

mat4 resolve_model_matrix()
{
    if (pc.use_instancing != 0u) {
        return pc_instances.blades[gl_InstanceIndex].model;
    }

    return pc.pc_data.model;
}

void main()
{
    mat4 model_matrix = resolve_model_matrix();

    vec3 local_pos = inPosition;

    vec3 instance_origin = model_matrix[3].xyz;
    float height_factor = clamp(inPosition.y, 0.0, 1.0);
    float stiffness = height_factor * height_factor;

    vec2 wind_dir = normalize(vec2(0.82, 0.57));
    float phase = dot(instance_origin.xz, vec2(0.11, 0.07));

    float gust = sin(timeUBO.time * 1.9 + phase * 6.28318);
    float flutter = sin(timeUBO.time * 5.2 + phase * 13.0 + inPosition.y * 4.0);

    float sway = (gust * 0.11 + flutter * 0.035) * stiffness;

    local_pos.x += wind_dir.x * sway;
    local_pos.z += wind_dir.y * sway;

    vec4 world_pos = model_matrix * vec4(local_pos, 1.0);

    gl_Position = cameraUBO.proj * cameraUBO.view * world_pos;
    fragPosWorld = world_pos.xyz;

    vec3 bent_normal = normalize(inNormal + vec3(wind_dir.x, 0.0, wind_dir.y) * (sway * 3.0));
    mat3 normal_matrix = transpose(inverse(mat3(model_matrix)));
    fragNormal = normalize(normal_matrix * bent_normal);

    fragTexCoord = inTexCoord;
}
