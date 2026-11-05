#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"
#include "include/edit.glsl"

// The edit cage's vertices -- six vertices a point, a square facing the camera, the same size on screen at any distance
layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_corner;  // -1..1 across the square -- edit.frag rounds it

void main() {
    uint point = uint(gl_VertexIndex) / 6u;
    vec2 corner = Ir77EditCorner(uint(gl_VertexIndex) % 6u) * vec2(2.0, 1.0) - vec2(1.0, 0.0);  // (along, across) -> -1..1 square

    Ir77EditPoint p = IR77_EDIT_POINTS.data[point];
    vec4 clip = Ir77EditClip(p);

    v_color = Ir77EditSelected(p) ? EDIT_SELECTED_COLOR : EDIT_POINT_COLOR;
    v_corner = corner;

    if (clip.w <= 1e-6) {
        gl_Position = EDIT_CULLED;
        return;
    }

    // clip units of the height, both ways -- x shrinks by the aspect so the square stays square
    clip.xy += corner * EDIT_POINT_HALF * vec2(1.0 / Ir77EditAspect(), 1.0) * clip.w;

    gl_Position = Ir77EditBias(clip);
}
