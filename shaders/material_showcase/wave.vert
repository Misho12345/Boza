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

struct WaveData {
    mat4 model;
};

layout(set = 0, binding = 5) readonly buffer PcInstances {
    WaveData instances[];
} pc_instances;

layout(push_constant) uniform PushConstants {
    WaveData push;
    uint use_instancing;
} pc;

mat4 resolve_model_matrix() {
    if (pc.use_instancing != 0u) {
        return pc_instances.instances[gl_InstanceIndex].model;
    }

    return pc.push.model;
}

void main() {
    vec3 pos = inPosition;

    float wave = sin(pos.x * 3.0 + timeUBO.time * 2.0) * 0.1;
    wave += sin(pos.z * 2.0 + timeUBO.time * 1.5) * 0.1;
    pos.y += wave;

    mat4 model_matrix = resolve_model_matrix();

    vec4 worldPos = model_matrix * vec4(pos, 1.0);
    gl_Position = cameraUBO.proj * cameraUBO.view * worldPos;
    fragPosWorld = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(model_matrix)));
    fragNormal = normalize(normalMatrix * inNormal);

    fragTexCoord = inTexCoord;
}

