#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"
#include "include/outline.glsl"

// The outline's shell (or mask) for vertex-skinned geometry -- posed as skinned.vert poses it (skinned BLEND surfaces draw this way)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 3) in uvec4 inJoints;
layout(location = 4) in vec4 inWeights;

void main() {
    uint base = IR77_BONES.bone_base[gl_InstanceIndex];

    mat4 skin = inWeights.x * IR77_BONES.bones[base + inJoints.x] +
                inWeights.y * IR77_BONES.bones[base + inJoints.y] +
                inWeights.z * IR77_BONES.bones[base + inJoints.z] +
                inWeights.w * IR77_BONES.bones[base + inJoints.w];

    mat4 model = IR77_MODEL(gl_InstanceIndex) * skin;

    vec4 clip = IR77_CAMERA.view_proj * (model * vec4(inPosition, 1.0));

    gl_Position = Ir77OutlinePlace(clip, mat3(model) * inNormal);
}
