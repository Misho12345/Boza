#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) flat out vec4 fragTint;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cameraUBO;

struct DrawData {
    mat4 model;
    vec4 tint;
};

layout(set = 0, binding = 5) readonly buffer PcInstances {
    DrawData items[];
} pc_instances;

layout(push_constant) uniform PushConstants {
    mat4 shadow_view_projection;
    DrawData payload;
    uint use_instancing;
    uint render_mode;
} pc;

DrawData resolve_draw_data()
{
    if (pc.use_instancing != 0u) {
        return pc_instances.items[gl_InstanceIndex];
    }

    return pc.payload;
}

void main()
{
    DrawData draw_data = resolve_draw_data();
    vec4 world_pos = draw_data.model * vec4(inPosition, 1.0);

    if (pc.render_mode != 0u) {
        gl_Position = pc.shadow_view_projection * world_pos;
    } else {
        vec4 view_pos = cameraUBO.view * world_pos;
        gl_Position = cameraUBO.proj * view_pos;
    }
    fragPosWorld = world_pos.xyz;

    mat3 normal_matrix = transpose(inverse(mat3(draw_data.model)));
    fragNormal = normalize(normal_matrix * inNormal);
    fragTint = draw_data.tint;
}
