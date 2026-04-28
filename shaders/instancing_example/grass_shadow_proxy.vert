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

struct GrassShadowData {
    mat4 model;
    vec4 sphere;
};

layout(set = 0, binding = 5) readonly buffer PcInstances {
    GrassShadowData items[];
} pc_instances;

layout(push_constant) uniform PushConstants {
    mat4 shadow_view_projection;
    GrassShadowData payload;
    uint use_instancing;
    uint render_mode;
} pc;

GrassShadowData resolve_draw_data()
{
    if (pc.use_instancing != 0u)
        return pc_instances.items[gl_InstanceIndex];

    return pc.payload;
}

void main()
{
    GrassShadowData draw_data = resolve_draw_data();
    vec4 world_pos = draw_data.model * vec4(inPosition, 1.0);

    if (pc.render_mode != 0u)
        gl_Position = pc.shadow_view_projection * world_pos;
    else
        gl_Position = cameraUBO.proj * cameraUBO.view * world_pos;

    fragPosWorld = world_pos.xyz;
    fragNormal = normalize(mat3(transpose(inverse(draw_data.model))) * inNormal);
    fragTexCoord = inTexCoord;
}
