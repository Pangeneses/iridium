#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"
#include "include/outline.glsl"

// The outline's one colour -- or, for the mask, nothing (alpha 0 through its alpha-blend pipeline: only its depth lands)
layout(location = 0) out vec4 o_color;

void main() {
    o_color = Ir77OutlineMask() ? vec4(0.0) : unpackUnorm4x8(IR77_OUTLINE_COLOR);
}
