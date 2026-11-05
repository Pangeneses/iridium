#version 460
#extension GL_GOOGLE_include_directive : require
#define IR77_STAGE_FRAGMENT
#include "include/bindless.glsl"
#include "include/field.glsl"
#line 1
// Thomas cyclically symmetric attractor -- every particle integrates from where it was last frame

// the velocity at point (x, y, z)
vec3 Thomas(float x, float y, float z, float b) {
    float dx = sin(y) - b * x;
    float dy = sin(z) - b * y;
    float dz = sin(x) - b * z;

    return vec3(dx, dy, dz);
}

Ir77FieldPoint Field(Ir77FieldIn f) {
    // sliders -> real values (0.5 = the middle of each range)
    float b = 0.10 + (0.30 - 0.10) * f.params[0];     // damping: ~0.208 is the classic lattice; lower = wilder
    float speed = 0.5 + (8.0 - 0.5) * f.params[1];    // how fast simulated time runs
    float scale = 0.4 + (1.2 - 0.4) * f.params[2];    // attractor units -> world units
    float opacity = 0.03 + (0.3 - 0.03) * f.params[3];  // particles add up where they overlap

    Ir77FieldPoint point = f.prev;  // where this particle was last frame

    // world -> attractor
    float x = point.position.x / scale;
    float y = point.position.y / scale;
    float z = point.position.z / scale;

    // does this particle need a new start?
    bool first_frame = f.first;
    bool broken = isnan(x) || isnan(y) || isnan(z);
    bool escaped = (x * x + y * y + z * z) > 400.0;  // more than 20 units out

    if (first_frame || broken || escaped) {
        // a random point in the cube -4..4 on every axis
        x = Ir77Random(f.id, 1u) * 8.0 - 4.0;
        y = Ir77Random(f.id, 2u) * 8.0 - 4.0;
        z = Ir77Random(f.id, 3u) * 8.0 - 4.0;

        point.size = 0.008;
    }

    // this frame's time, split into 4 smaller steps
    float frame_time = min(f.dt, 0.02);
    float step = frame_time * speed / 4.0;

    vec3 velocity = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < 4; i++) {
        velocity = Thomas(x, y, z, b);

        x = x + velocity.x * step;
        y = y + velocity.y * step;
        z = z + velocity.z * step;
    }

    // attractor -> world
    point.position = vec3(x * scale, y * scale, z * scale);

    // colour = the direction of travel: red along x, green along y, blue along z
    float length_of_velocity = length(velocity);
    vec3 direction = vec3(0.0, 0.0, 0.0);
    if (length_of_velocity > 0.0) direction = abs(velocity) / length_of_velocity;

    vec3 colour = vec3(0.15, 0.15, 0.15) + direction * 0.85;

    point.color = vec4(colour.x, colour.y, colour.z, opacity);
    point.data = vec4(length_of_velocity, 0.0, 0.0, 0.0);  // data.x = speed, for the colormap shading

    return point;
}
#include "include/field_fragment.glsl"
