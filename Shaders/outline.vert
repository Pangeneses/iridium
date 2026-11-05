#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"
#include "include/outline.glsl"

// The outline's shell (or mask) for static geometry and the compute-skinned posed stream -- reads the static layout's position and normal
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

void main() {
    mat4 model = IR77_MODEL(gl_InstanceIndex);

    vec4 clip = IR77_CAMERA.view_proj * (model * vec4(inPosition, 1.0));

    gl_Position = Ir77OutlinePlace(clip, mat3(model) * inNormal);
}
