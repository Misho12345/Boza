#version 450
#include "../common/fbm.glsl"

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

layout(set = 0, binding = 6) uniform GrassSettingsUBO {
    vec2 sway_direction;
    float sway_strength;
} grassSettings;

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
    mat4 modelMatrix = resolve_model_matrix();

    vec3 bladeWorldPos = vec3(modelMatrix[3][0], modelMatrix[3][1], modelMatrix[3][2]);

    float heightFactor = inPosition.y / 1.6;
    float heightFactorQuad = heightFactor * heightFactor;

    vec2 noiseCoord = bladeWorldPos.xz * 0.15;
    float noiseBasis = perlin2(noiseCoord, 12345u) * 0.5 + 0.5;

    float timeWave = sin(timeUBO.time * 1.5) * 0.25 + 0.75;

    float timeOffset = perlin2(bladeWorldPos.xz * 0.08 + vec2(timeUBO.time * 0.1), 54321u);
    float irregularWave = sin((timeUBO.time + timeOffset * 2.0) * 1.3) * 0.25 + 0.75;

    float swayMultiplier = mix(timeWave, irregularWave, noiseBasis);

    float noiseX = perlin2(noiseCoord + vec2(0.0, timeUBO.time * 0.25), 11111u);
    float noiseZ = perlin2(noiseCoord + vec2(100.0, timeUBO.time * 0.25), 22222u);

    vec2 noisyDirection = grassSettings.sway_direction + vec2(noiseX, noiseZ) * 0.3;
    noisyDirection = normalize(noisyDirection);

    float baseSway = swayMultiplier * grassSettings.sway_strength * heightFactorQuad;

    float waveFrequency = 0.7;
    float waveSpeed = 3.0;

    float bladePhase = perlin2(bladeWorldPos.xz * 0.2, 99999u) * 6.28318;

    float wavePhase = heightFactor * waveFrequency * 6.28318 - timeUBO.time * waveSpeed + bladePhase;
    float wave = sin(wavePhase) * 0.05;

    float waveFactor = smoothstep(0.2, 0.8, heightFactor);

    float totalSway = baseSway * (1.0 + wave * waveFactor);

    vec3 swayOffset = vec3(
        noisyDirection.x * totalSway,
        0.0,
        noisyDirection.y * totalSway
    );

    vec3 swayedPosition = inPosition + swayOffset;

    vec4 worldPos = modelMatrix * vec4(swayedPosition, 1.0);
    fragPosWorld = worldPos.xyz;

    gl_Position = cameraUBO.proj * cameraUBO.view * worldPos;

    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));
    fragNormal = normalize(normalMatrix * inNormal);

    fragTexCoord = inTexCoord;
}