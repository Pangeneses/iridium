#version 460
#extension GL_GOOGLE_include_directive : require
#define IR77_STAGE_FRAGMENT
#include "include/bindless.glsl"
#include "include/field.glsl"
#line 1
// Fire -- particles born on a disc at the base, lifted by heat, buffeted by turbulence, cooling from white to red to smoke, then reborn.
//   data.x    = age, 0 at birth .. 1 at death
//   data.yzw  = velocity

// A swirling push that changes with position and time -- what makes the flames flicker
vec3 Turbulence(vec3 p, float time) {
    float x = sin(p.y * 3.1 + time * 2.3) + 0.5 * sin(p.z * 5.7 - time * 1.7);
    float y = 0.3 * sin(p.x * 4.3 + time * 1.9);
    float z = sin(p.y * 2.7 - time * 2.9) + 0.5 * sin(p.x * 6.1 + time * 1.3);

    return vec3(x, y, z);
}

// A random point on the base disc
vec3 SpawnPoint(uvec3 id, uint seed, float radius) {
    float pi = 3.14159265;
    float angle = Ir77Random(id, seed) * 2.0 * pi;
    float distance = sqrt(Ir77Random(id, seed + 1u)) * radius;  // sqrt spreads them evenly over the disc

    return vec3(cos(angle) * distance, 0.0, sin(angle) * distance);
}

// Colour by age: white-yellow -> orange -> red -> dark smoke
vec3 FireColour(float age) {
    vec3 white = vec3(1.0, 0.95, 0.7);
    vec3 orange = vec3(1.0, 0.5, 0.1);
    vec3 red = vec3(0.8, 0.1, 0.02);
    vec3 smoke = vec3(0.1, 0.1, 0.1);

    if (age < 0.15) return white + (orange - white) * (age / 0.15);
    if (age < 0.5) return orange + (red - orange) * ((age - 0.15) / 0.35);
    return red + (smoke - red) * ((age - 0.5) / 0.5);
}

Ir77FieldPoint Field(Ir77FieldIn f) {
    // sliders -> real values (0.5 = the middle of each range)
    float lift = 0.5 + (6.0 - 0.5) * f.params[0];          // how hard heat pushes up
    float turbulence = 0.0 + (4.0 - 0.0) * f.params[1];    // how much the flames flicker
    float lifetime = 0.5 + (3.0 - 0.5) * f.params[2];      // seconds a particle lives
    float base_radius = 0.1 + (1.5 - 0.1) * f.params[3];   // size of the burning disc
    float pinch = 0.0 + (3.0 - 0.0) * f.params[4];         // pull toward the centre -- the flame's tip
    float opacity = 0.01 + (0.2 - 0.01) * f.params[5];     // how bright each particle is
    float size = 0.01 + (0.08 - 0.01) * f.params[6];       // particle size at birth

    Ir77FieldPoint point = f.prev;

    float age = point.data.x;
    vec3 velocity = point.data.yzw;
    vec3 position = point.position;

    // this particle's own lifetime -- 60% .. 140% of the slider, so they don't all die together
    float my_lifetime = lifetime * (0.6 + 0.8 * Ir77Random(f.id, 4u));

    float dt = min(f.dt, 0.033);

    // first frame: born at the base, at a random point in its life, so the fire starts already burning
    if (f.first) {
        position = SpawnPoint(f.id, 1u, base_radius);
        velocity = vec3(0.0, 0.0, 0.0);
        age = Ir77Random(f.id, 5u);
    }

    // died: reborn at the base -- the seed changes with time, so it lands somewhere new each life
    if (age >= 1.0) {
        uint seed = uint(f.time * 997.0);
        position = SpawnPoint(f.id, seed, base_radius);
        velocity = vec3(0.0, 0.0, 0.0);
        age = 0.0;
    }

    // forces
    vec3 buoyancy = vec3(0.0, lift, 0.0);
    vec3 swirl = Turbulence(position, f.time) * turbulence;
    vec3 toward_centre = vec3(-position.x, 0.0, -position.z) * pinch;

    velocity = velocity + (buoyancy + swirl + toward_centre) * dt;

    // air drag -- velocity loses 2/3 every second, so particles don't speed up forever
    velocity = velocity * exp(-1.1 * dt);

    position = position + velocity * dt;
    age = age + dt / my_lifetime;

    // look: colour by age; fade in over the first 5% of life, fade out over the last 40%; grow as it cools
    float fade_in = age / 0.05;
    if (fade_in > 1.0) fade_in = 1.0;

    float fade_out = (1.0 - age) / 0.4;
    if (fade_out > 1.0) fade_out = 1.0;
    if (fade_out < 0.0) fade_out = 0.0;

    vec3 colour = FireColour(age);

    point.position = position;
    point.size = size * (1.0 + 2.0 * age);
    point.color = vec4(colour.x, colour.y, colour.z, opacity * fade_in * fade_out);
    point.data = vec4(age, velocity.x, velocity.y, velocity.z);

    return point;
}
#include "include/field_fragment.glsl"
