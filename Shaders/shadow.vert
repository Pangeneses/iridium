#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

layout(location = 0) in vec3 inPosition;

// depth only -- pc.user[0] = which shadow matrix
void main() {
    gl_Position = IR77_SHADOW_MATRIX(pc.user[0]) * IR77_MODEL(gl_InstanceIndex) * vec4(inPosition, 1.0);
}
