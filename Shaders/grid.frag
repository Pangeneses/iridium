#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

// The editor grid -- where this pixel's ray meets the grid plane through the origin:
//   perspective: the ground, XZ
//   orthographic: the plane facing the camera -- top XZ, front XY, side ZY (the axis it looks down most)
// Line spacing follows the zoom in powers of 10: the finest lines sit about 10 pixels apart and fade out as they close up, the next level
// takes over. Axes through the origin: X red, Y green, Z blue. Depth is the hit's, so geometry hides it; past the far plane it sits just in
// front of the clear, behind everything.
layout(location = 0) noperspective in vec4 v_near;
layout(location = 1) noperspective in vec4 v_far;

layout(location = 0) out vec4 o_color;

const float GRID_PIXELS = 10.0;           // the finest lines at least this far apart
const vec3 GRID_LINE = vec3(0.35);        // ordinary lines
const vec3 GRID_X = vec3(0.85, 0.2, 0.25);
const vec3 GRID_Y = vec3(0.45, 0.8, 0.2);
const vec3 GRID_Z = vec3(0.2, 0.45, 0.9);

// 1 on a line of this spacing, 0 a pixel away -- anti-aliased by the coordinate's screen rate
float Lines(vec2 coord, vec2 rate, float spacing) {
    vec2 g = coord / spacing;
    vec2 d = abs(fract(g - 0.5) - 0.5) / max(rate / spacing, vec2(1e-8));
    return 1.0 - min(min(d.x, d.y), 1.0);
}

void main() {
    vec3 a = v_near.xyz / v_near.w;
    vec3 b = v_far.xyz / v_far.w;
    vec3 ray = normalize(b - a);

    bool ortho = IR77_CAMERA.proj[3][3] == 1.0;
    vec3 forward = -vec3(IR77_CAMERA.view[0][2], IR77_CAMERA.view[1][2], IR77_CAMERA.view[2][2]);
    vec3 f = abs(forward);

    // 0 = XZ, 1 = XY, 2 = ZY
    int plane = !ortho || (f.y >= f.x && f.y >= f.z) ? 0 : f.z >= f.x ? 1 : 2;
    vec3 n = plane == 0 ? vec3(0, 1, 0) : plane == 1 ? vec3(0, 0, 1) : vec3(1, 0, 0);

    float facing = dot(ray, n);
    if (abs(facing) < 1e-6) discard;

    float t = -dot(a, n) / facing;
    if (t < 0.0) discard;

    vec3 hit = a + ray * t;
    vec2 coord = plane == 0 ? hit.xz : plane == 1 ? hit.xy : hit.zy;
    vec2 rate = fwidth(coord);

    // world units per pixel -> the level; fract = how far the finest lines have closed up
    float pixel = max(max(rate.x, rate.y), 1e-8);
    float level = log(pixel * GRID_PIXELS) / log(10.0);
    float base = pow(10.0, floor(level));
    float fade = fract(level);

    float alpha = max(max(Lines(coord, rate, base) * (1.0 - fade) * 0.35, Lines(coord, rate, base * 10.0) * 0.6), Lines(coord, rate, base * 100.0) * 0.8);
    vec3 color = GRID_LINE;

    // the axes -- the line along the plane's first axis (second coordinate 0), then along its second (first coordinate 0)
    vec2 axis = abs(coord) / max(rate, vec2(1e-8));
    if (axis.y < 1.0) {
        color = plane == 2 ? GRID_Z : GRID_X;
        alpha = 1.0 - axis.y;
    }
    if (axis.x < 1.0) {
        color = plane == 0 ? GRID_Z : GRID_Y;
        alpha = 1.0 - axis.x;
    }

    // perspective: thin out as the ray grazes the plane -- no moire toward the horizon
    if (!ortho) alpha *= clamp(abs(facing) * 4.0, 0.0, 1.0);

    if (alpha <= 0.001) discard;

    vec4 clip = IR77_CAMERA.view_proj * vec4(hit, 1.0);
    gl_FragDepth = clamp(clip.z / clip.w, 1e-7, 1.0);  // reversed-Z: past the far plane -> just in front of the clear (0)

    o_color = vec4(color, alpha);
}
