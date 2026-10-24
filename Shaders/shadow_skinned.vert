#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 3) in uvec4 inJoints;
layout(location = 4) in vec4 inWeights;

// depth only -- pc.user[0] = which shadow matrix; same skinning as skinned.vert
void main() {
    uint base = IR77_BONES.bone_base[gl_InstanceIndex];

    mat4 skin = inWeights.x * IR77_BONES.bones[base + inJoints.x] +
                inWeights.y * IR77_BONES.bones[base + inJoints.y] +
                inWeights.z * IR77_BONES.bones[base + inJoints.z] +
                inWeights.w * IR77_BONES.bones[base + inJoints.w];

    gl_Position = IR77_SHADOW_MATRIX(pc.user[0]) * IR77_MODEL(gl_InstanceIndex) * skin * vec4(inPosition, 1.0);
}
