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
} cam;

layout(set = 0, binding = 4) uniform TimeUBO {
    float time;
    float delta_time;
    vec2 padding;
} timeData;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {
    vec3 pos = inPosition;

    float wave = sin(pos.x * 3.0 + timeData.time * 2.0) * 0.1;
    wave += sin(pos.z * 2.0 + timeData.time * 1.5) * 0.1;
    pos.y += wave;

    vec4 worldPos = pc.model * vec4(pos, 1.0);
    gl_Position = cam.proj * cam.view * worldPos;
    fragPosWorld = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(pc.model)));
    fragNormal = normalize(normalMatrix * inNormal);

    fragTexCoord = inTexCoord;
}

