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

struct DrawData {
    mat4 model;
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

mat4 resolve_model_matrix() {
    if (pc.use_instancing != 0u) {
        return pc_instances.items[gl_InstanceIndex].model;
    }

    return pc.payload.model;
}

void main() {
    mat4 model_matrix = resolve_model_matrix();

    vec4 worldPos = model_matrix * vec4(inPosition, 1.0);
    if (pc.render_mode != 0u) {
        gl_Position = pc.shadow_view_projection * worldPos;
    } else {
        vec4 viewPos = cameraUBO.view * worldPos;
        gl_Position = cameraUBO.proj * viewPos;
    }
    fragPosWorld = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(model_matrix)));
    fragNormal = normalize(normalMatrix * inNormal);

    fragTexCoord = inTexCoord;
}
