#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cameraUBO;

struct UnlitData {
    mat4 model;
};

layout(set = 0, binding = 5) readonly buffer PcInstances {
    UnlitData records[];
} pc_instances;

layout(push_constant) uniform PushConstants {
    UnlitData params;
    uint use_instancing;
} pc;

mat4 resolve_model_matrix() {
    if (pc.use_instancing != 0u) {
        return pc_instances.records[gl_InstanceIndex].model;
    }

    return pc.params.model;
}

void main() {
    mat4 model_matrix = resolve_model_matrix();

    gl_Position = cameraUBO.proj * cameraUBO.view * model_matrix * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord + clamp(inNormal.x, 0.0f, 1.0f) * 0.0f;
}
