#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in uvec4 inJoints;
layout(location = 4) in vec4 inWeights;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec3 fragWorldPos;

void main() {
    uint base = IR77_BONES.bone_base[gl_InstanceIndex];

    // up to four joints per vertex; glTF weights sum to 1
    mat4 skin = inWeights.x * IR77_BONES.bones[base + inJoints.x] +
                inWeights.y * IR77_BONES.bones[base + inJoints.y] +
                inWeights.z * IR77_BONES.bones[base + inJoints.z] +
                inWeights.w * IR77_BONES.bones[base + inJoints.w];

    mat4 model = IR77_MODEL(gl_InstanceIndex) * skin;

    vec4 world = model * vec4(inPosition, 1.0);

    fragWorldPos = world.xyz;
    fragNormal = mat3(model) * inNormal;
    fragUV = inUV;

    gl_Position = IR77_CAMERA.view_proj * world;
}
