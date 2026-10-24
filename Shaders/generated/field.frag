#version 460
#extension GL_GOOGLE_include_directive : require
#define IR77_STAGE_FRAGMENT
#include "include/bindless.glsl"
#include "include/field.glsl"
#line 1
// Halvorsen attractor -- every particle integrates from where it was last frame

// the velocity at point (x, y, z)
vec3 Halvorsen(float x, float y, float z, float a) {
    float dx = -a * x - 4.0 * y - 4.0 * z - y * y;
    float dy = -a * y - 4.0 * z - 4.0 * x - z * z;
    float dz = -a * z - 4.0 * x - 4.0 * y - x * x;

    return vec3(dx, dy, dz);
}

Ir77FieldPoint Field(Ir77FieldIn f) {
    // sliders -> real values (0.5 = the middle of each range)
    float a = 1.4 + (2.4 - 1.4) * f.params[0];          // damping: 1.89 is the classic shape
    float speed = 0.1 + (1.5 - 0.1) * f.params[1];      // how fast simulated time runs
    float scale = 0.15 + (0.45 - 0.15) * f.params[2];   // attractor units -> world units
    float opacity = 0.03 + (0.3 - 0.03) * f.params[3];  // particles add up where they overlap

    Ir77FieldPoint point = f.prev;  // where this particle was last frame

    // world -> attractor
    float x = point.position.x / scale;
    float y = point.position.y / scale;
    float z = point.position.z / scale;

    // does this particle need a new start?
    bool first_frame = f.first;
    bool broken = isnan(x) || isnan(y) || isnan(z);
    bool escaped = (x * x + y * y + z * z) > 1600.0;  // more than 40 units out

    if (first_frame || broken || escaped) {
        // a random point in the cube -2..2 on every axis
        x = Ir77Random(f.id, 1u) * 4.0 - 2.0;
        y = Ir77Random(f.id, 2u) * 4.0 - 2.0;
        z = Ir77Random(f.id, 3u) * 4.0 - 2.0;

        point.size = 0.008;
    }

    // this frame's time, split into 8 smaller steps -- Halvorsen moves fast, small steps keep it on the curve
    float frame_time = min(f.dt, 0.016);
    float step = frame_time * speed / 8.0;

    vec3 velocity = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < 8; i++) {
        velocity = Halvorsen(x, y, z, a);

        x = x + velocity.x * step;
        y = y + velocity.y * step;
        z = z + velocity.z * step;
    }

    // attractor -> world
    point.position = vec3(x * scale, y * scale, z * scale);

    // colour = distance from the centre: purple at the core, yellow out on the lobes
    float distance = sqrt(x * x + y * y + z * z);
    float t = distance / 12.0;
    if (t > 1.0) t = 1.0;

    vec3 core_colour = vec3(0.45, 0.1, 0.8);
    vec3 outer_colour = vec3(1.0, 0.85, 0.2);
    vec3 colour = core_colour + (outer_colour - core_colour) * t;

    point.color = vec4(colour.x, colour.y, colour.z, opacity);
    point.data = vec4(t, 0.0, 0.0, 0.0);  // data.x = distance 0..1, for the colormap shading

    return point;
}
#include "include/field_fragment.glsl"
