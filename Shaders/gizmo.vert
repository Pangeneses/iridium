#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"
#include "include/edit.glsl"

// The transform gizmo -- translate arrows, rotate rings or scale boxes at pc.model's translation (the selection's median in edit mode, the
// active entity in object mode), along pc.model's axes (its unit columns -- the world's, or an object's own for a scale), drawn on top of
// everything at a fixed size on screen. Six vertices a segment, a screen-space strip like an edge. Must match Ir77Gizmo::Segments; the centre
// square always faces the camera.
//   pc.user[4] = mode (GIZMO_MODE_*), pc.user[5] = the lit handle (0 X, 1 Y, 2 Z, 3 centre; anything else: none)
layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_corner;  // (0, 0) -- not rounded

#define GIZMO_MODE_TRANSLATE 0u
#define GIZMO_MODE_ROTATE 1u
#define GIZMO_MODE_SCALE 2u

#define GIZMO_CENTRE 3u

const float GIZMO_FRACTION = 0.22;       // of half the view's height -- Ir77Types.hpp GIZMO_FRACTION
const uint GIZMO_RING_SEGMENTS = 48u;
const float GIZMO_HEAD_LENGTH = 0.2;
const float GIZMO_HEAD_WIDTH = 0.07;
const float GIZMO_BOX = 0.06;
const float GIZMO_SQUARE = 0.08;
const float GIZMO_HALF_WIDTH = 0.0028;   // half a line's width, clip units of the view's height (~3.5 px wide at 1300 px tall)

const vec4 GIZMO_COLORS[4] = vec4[4](vec4(0.9, 0.2, 0.25, 1.0), vec4(0.45, 0.85, 0.2, 1.0), vec4(0.2, 0.45, 0.95, 1.0), vec4(0.92, 0.92, 0.92, 1.0));
const vec4 GIZMO_HOT = vec4(1.0, 0.85, 0.15, 1.0);

vec3 Axis(uint a) { return a == 0u ? vec3(1, 0, 0) : a == 1u ? vec3(0, 1, 0) : vec3(0, 0, 1); }

// an axis's two sides -- the ring's plane, the box's plane, the arrowhead's spread
void Sides(uint a, out vec3 p, out vec3 q) {
    p = a == 0u ? vec3(0, 1, 0) : vec3(1, 0, 0);
    q = a == 2u ? vec3(0, 1, 0) : vec3(0, 0, 1);
}

// edge k of a square around c in the p / q plane
void Square(vec3 c, vec3 p, vec3 q, float half_size, uint k, out vec3 a, out vec3 b) {
    vec3 corners[4] = vec3[4](c + (-p - q) * half_size, c + (p - q) * half_size, c + (p + q) * half_size, c + (-p + q) * half_size);
    a = corners[k];
    b = corners[(k + 1u) % 4u];
}

// segment s of the mode -- its ends in gizmo units and its handle
void Segment(uint mode, uint s, vec3 right, vec3 up, out vec3 a, out vec3 b, out uint handle) {
    if (mode == GIZMO_MODE_ROTATE) {
        uint axis = s / GIZMO_RING_SEGMENTS;
        uint k = s % GIZMO_RING_SEGMENTS;
        vec3 p, q;
        Sides(axis, p, q);

        float t0 = 6.28318530718 * float(k) / float(GIZMO_RING_SEGMENTS);
        float t1 = 6.28318530718 * float(k + 1u) / float(GIZMO_RING_SEGMENTS);
        a = p * cos(t0) + q * sin(t0);
        b = p * cos(t1) + q * sin(t1);
        handle = axis;
        return;
    }

    uint per_axis = mode == GIZMO_MODE_TRANSLATE ? 3u : 5u;

    if (s >= per_axis * 3u) {  // the centre square, facing the camera
        Square(vec3(0.0), right, up, GIZMO_SQUARE, s - per_axis * 3u, a, b);
        handle = GIZMO_CENTRE;
        return;
    }

    uint axis_index = s / per_axis;
    uint k = s % per_axis;
    vec3 axis = Axis(axis_index);
    vec3 p, q;
    Sides(axis_index, p, q);
    handle = axis_index;

    if (k == 0u) {  // the shaft
        a = vec3(0.0);
        b = axis;
    } else if (mode == GIZMO_MODE_TRANSLATE) {  // the arrowhead
        a = axis;
        b = axis * (1.0 - GIZMO_HEAD_LENGTH) + p * (k == 1u ? GIZMO_HEAD_WIDTH : -GIZMO_HEAD_WIDTH);
    } else {  // the box at the tip
        Square(axis, p, q, GIZMO_BOX, k - 1u, a, b);
    }
}

void main() {
    uint segment = uint(gl_VertexIndex) / 6u;
    vec2 corner = Ir77EditCorner(uint(gl_VertexIndex) % 6u);  // x along 0..1, y across -1..1

    vec3 centre = pc.model[3].xyz;
    vec3 right = vec3(IR77_CAMERA.view[0][0], IR77_CAMERA.view[1][0], IR77_CAMERA.view[2][0]);
    vec3 up = vec3(IR77_CAMERA.view[0][1], IR77_CAMERA.view[1][1], IR77_CAMERA.view[2][1]);

    vec3 a, b;
    uint handle;
    Segment(pc.user[4], segment, right, up, a, b, handle);

    // along the placement's axes -- all but the centre square
    if (handle != GIZMO_CENTRE) {
        mat3 basis = mat3(pc.model);
        a = basis * a;
        b = basis * b;
    }

    // a fixed size on screen -- GIZMO_FRACTION of half the view's height at the centre's depth
    float w = (IR77_CAMERA.view_proj * vec4(centre, 1.0)).w;
    float size = GIZMO_FRACTION * max(w, 1e-6) / max(abs(IR77_CAMERA.proj[1][1]), 1e-6);

    vec4 ca = IR77_CAMERA.view_proj * vec4(centre + a * size, 1.0);
    vec4 cb = IR77_CAMERA.view_proj * vec4(centre + b * size, 1.0);

    v_color = handle == pc.user[5] ? GIZMO_HOT : GIZMO_COLORS[min(handle, 3u)];
    v_corner = vec2(0.0);

    if (ca.w <= 1e-6 || cb.w <= 1e-6) {
        gl_Position = EDIT_CULLED;
        return;
    }

    float aspect = Ir77EditAspect();
    vec2 square = vec2(aspect, 1.0);

    vec2 along = (cb.xy / cb.w - ca.xy / ca.w) * square;
    vec2 dir = dot(along, along) > 1e-12 ? normalize(along) : vec2(1.0, 0.0);
    vec2 normal = vec2(-dir.y, dir.x);

    vec4 clip = mix(ca, cb, corner.x);
    clip.xy += normal / square * (corner.y * GIZMO_HALF_WIDTH) * clip.w;

    gl_Position = clip;
}
