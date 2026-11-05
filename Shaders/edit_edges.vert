#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"
#include "include/edit.glsl"

// The edit cage's edges -- six vertices an edge, a strip between its two points, a constant width on screen
layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_corner;  // (0, 0) -- edges are not rounded

void main() {
    uint edge = uint(gl_VertexIndex) / 6u;
    vec2 corner = Ir77EditCorner(uint(gl_VertexIndex) % 6u);  // x along the edge 0..1, y across -1..1

    uvec2 ends = IR77_EDIT_EDGES.data[edge];
    Ir77EditPoint pa = IR77_EDIT_POINTS.data[ends.x];
    Ir77EditPoint pb = IR77_EDIT_POINTS.data[ends.y];
    vec4 a = Ir77EditClip(pa);
    vec4 b = Ir77EditClip(pb);

    v_color = Ir77EditSelected(pa) && Ir77EditSelected(pb) ? EDIT_SELECTED_COLOR : EDIT_EDGE_COLOR;
    v_corner = vec2(0.0);

    // an end behind the eye -- left out rather than clipped by hand
    if (a.w <= 1e-6 || b.w <= 1e-6) {
        gl_Position = EDIT_CULLED;
        return;
    }

    // the edge's direction on screen, in square units (x times the aspect), and its normal there
    float aspect = Ir77EditAspect();
    vec2 square = vec2(aspect, 1.0);

    vec2 along = (b.xy / b.w - a.xy / a.w) * square;
    vec2 dir = dot(along, along) > 1e-12 ? normalize(along) : vec2(1.0, 0.0);
    vec2 normal = vec2(-dir.y, dir.x);

    vec4 clip = mix(a, b, corner.x);
    clip.xy += normal / square * (corner.y * EDIT_EDGE_HALF) * clip.w;

    gl_Position = Ir77EditBias(clip);
}
