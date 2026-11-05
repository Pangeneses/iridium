#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"
#include "include/edit.glsl"

// The edit cage's colour -- points come out round (v_corner -1..1 across the square), edges as they are
layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_corner;

layout(location = 0) out vec4 o_color;

void main() {
    float r = length(v_corner);
    if (r > 1.0) discard;

    o_color = v_color;
}
