#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"
#include "include/edit.glsl"

// The edit cage's selected faces -- three vertices a face, tinted where all three corners are selected; the rest are left out
layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_corner;  // (0, 0) -- faces are not rounded

void main() {
    uint face = uint(gl_VertexIndex) / 3u;
    uint corner = uint(gl_VertexIndex) % 3u;

    uvec4 corners = IR77_EDIT_FACES.data[face];
    Ir77EditPoint p0 = IR77_EDIT_POINTS.data[corners.x];
    Ir77EditPoint p1 = IR77_EDIT_POINTS.data[corners.y];
    Ir77EditPoint p2 = IR77_EDIT_POINTS.data[corners.z];

    v_color = EDIT_FACE_COLOR;
    v_corner = vec2(0.0);

    if (!(Ir77EditSelected(p0) && Ir77EditSelected(p1) && Ir77EditSelected(p2))) {
        gl_Position = EDIT_CULLED;
        return;
    }

    vec4 clip = Ir77EditClip(corner == 0u ? p0 : corner == 1u ? p1 : p2);

    gl_Position = Ir77EditBias(clip);
}
