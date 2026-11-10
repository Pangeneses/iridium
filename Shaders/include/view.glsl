// =============================================================================================================================================
// view.glsl -- generic views of a function: the program supplies only the function, this supplies Field(). The panel's topology picks the
// view:
//   particles  -- a cloud: every point walks (Metropolis -- a Gaussian step tried WALK_STEPS times a frame, kept with the odds weight new /
//                 weight old), so the points lie in proportion to the weight and shimmer as they go. Coloured by the value's sign.
//   surface    -- a slice: the grid laid on a plane through the function (world XY, XZ or YZ, set off along its normal), each point's value
//                 for the colour map / contours, the sheet lifted off the plane by it if asked.
//   isosurface -- the grid through the domain's box; the surface is where |value| = level, coloured by the sign of the lobe it wraps.
//                 Sweeping the level (a log slider) walks out through the function's layers.
// Slice and isosurface are recomputed only when the function or the view changes.
//
// A program is:
//   #include "include/<whatever the function needs>.glsl"
//   Ir77Sample Sample(vec3 p, float params[8]) { ... }   -- the function at p (the function's own units), from params[0..2]
//   #include "include/view.glsl"
//
// params[0..2] are the function's; params[3..7] the view's -- real values, as the panel's sliders send them. The program declares every
// slider it wants, each on its own comment line (the panel reads them; the compiler skips them):
//   // @param <index> <label> <min> <max> <default> [log] [step=<s>] [surface] [particles] [isosurface]
// log: the slider moves evenly in log(value) (min > 0); step: its increments (linear only -- (max - min) / 1000 if left out); topology
// names: shown only under those topologies (none: always). A label's '_' shows as a space. The view's, to copy into a program:
//   // @param 3 brightness 0.001 1 0.04 log particles
//   // @param 4 scale 0.05 20 1 log
//   // @param 5 size 0.001 0.5 0.015 log particles
//   // @param 6 step 0.01 4 0.3 log particles
//   // @param 7 cut 0 3 0 step=1 particles isosurface
//   // @param 3 height -10 10 0 surface
//   // @param 5 plane 0 2 0 step=1 surface
//   // @param 6 offset -20 20 0 surface
//   // @param 3 level 0.0001 1 0.02 log isosurface
//   cloud:      p[3] point alpha, p[4] scale (world units per function unit), p[5] point size, p[6] walk step (function units), p[7] cut
//               away: 0 nothing, 1 the +x half, 2 +y, 3 +z
//   slice:      p[3] lift per unit of value (0 flat), p[4] scale, p[5] plane 0 XY / 1 XZ / 2 YZ, p[6] offset along its normal
//   isosurface: p[3] level (|value| there), p[4] scale, p[7] cut away as the cloud's (the cut capped)
// A program's Marker() (field.glsl) is in the function's units here -- the view scales it with the rest.
// The domain (x / y / z ranges) is where the cloud's points start, the slice's extent on its plane (x, y), and the isosurface's box.
// =============================================================================================================================================
#ifndef IR77_VIEW_GLSL
#define IR77_VIEW_GLSL

#define VIEW_WALK_STEPS 4u

#define VIEW_PLUS vec3(1.0, 0.5, 0.15)   // a positive value's colour
#define VIEW_MINUS vec3(0.2, 0.5, 1.0)   // a negative one's

uint Ir77ViewTopology() { return ir77_field_configs[IR77_FIELD_CONFIG_SLOT].data[0].dims.w; }

vec4 Ir77ViewDomainX() { return ir77_field_configs[IR77_FIELD_CONFIG_SLOT].data[0].domain_x; }

vec4 Ir77ViewDomainZ() { return ir77_field_configs[IR77_FIELD_CONFIG_SLOT].data[0].domain_z; }

// what the function is -- its params and its data (the count and the first values): a change starts the cloud over / rebuilds the slice
float Ir77ViewFunctionKey(float params[8]) {
    float key = params[0] * 1.37 + params[1] * 2.71 + params[2] * 3.19 + float(Ir77FieldDataCount()) * 0.0007;

    uint n = min(Ir77FieldDataCount(), 16u);
    for (uint i = 0u; i < n; i++) key += Ir77FieldData(i) * (0.11 + 0.013 * float(i));

    return key;
}

// the function and the view together -- a slice / isosurface is rebuilt when this changes
float Ir77ViewKey(float params[8]) {
    return Ir77ViewFunctionKey(params) + params[3] * 5.03 + params[4] * 7.07 + params[5] * 11.3 + params[6] * 13.1 + params[7] * 17.9;
}

// world units per function unit -- 1 when the slider is missing (0)
float Ir77ViewScale(float params[8]) { return params[4] > 0.0 ? params[4] : 1.0; }

// p[7] -- a half cut away: 0 none, 1 +x, 2 +y, 3 +z
bool Ir77ViewCut(vec3 p, float params[8]) {
    int axis = int(params[7] + 0.5);
    return axis >= 1 && axis <= 3 && p[axis - 1] > 0.0;
}

#ifdef IR77_HAS_MARKERS
// the program's markers in world units -- its Marker() is in the function's, like Sample()
#define IR77_FIELD_MARKER_WORLD
vec4 Ir77FieldMarkerWorld(uint i, float p[8], out vec4 color) {
    vec4 m = Marker(i, p, color);
    float scale = Ir77ViewScale(p);
    return vec4(m.xyz * scale, m.w * scale);
}
#endif

// a normal (Gaussian) random 3-vector -- Box-Muller on stable per-element randoms
vec3 Ir77ViewNormal3(uvec3 id, uint seed) {
    float u1 = max(Ir77Random(id, seed), 1e-7);
    float u2 = Ir77Random(id, seed + 1u);
    float u3 = max(Ir77Random(id, seed + 2u), 1e-7);
    float u4 = Ir77Random(id, seed + 3u);

    float a = sqrt(-2.0 * log(u1));
    float b = sqrt(-2.0 * log(u3));
    return vec3(a * cos(6.2831853 * u2), a * sin(6.2831853 * u2), b * cos(6.2831853 * u4));
}

// ---- particles: the cloud ---------------------------------------------------------------------------------------------------------------

Ir77FieldPoint Ir77Cloud(Ir77FieldIn f) {
    float alpha = clamp(f.params[3], 0.0, 1.0);
    float scale = Ir77ViewScale(f.params);
    float size = max(f.params[5], 0.0);
    float reach = max(f.params[6], 1e-4);

    float key = Ir77ViewFunctionKey(f.params);

    vec3 p;  // where the point is -- the function's units
    Ir77Sample here;

    // the first frame, or a different function: start spread through the domain
    if (f.first || f.prev.data.z != key) {
        vec4 dx = Ir77ViewDomainX();
        vec4 dz = Ir77ViewDomainZ();
        vec3 cell = vec3(dx.y - dx.x, dx.w - dx.z, dz.y - dz.x) / vec3(max(f.dims, uvec3(1u)));
        p = f.coord + (vec3(Ir77Random(f.id, 11u), Ir77Random(f.id, 12u), Ir77Random(f.id, 13u)) - 0.5) * cell;
        here = Sample(p, f.params);
    } else {
        p = f.prev.position / max(f.prev.data.w, 1e-6);  // drawn scaled -- back to the function's units
        here.value = f.prev.data.x;
        here.weight = f.prev.data.y;
    }

    // the walk -- a Gaussian try, kept with the odds new / old (always, from nothing)
    uint frame = floatBitsToUint(f.time);
    for (uint s = 0u; s < VIEW_WALK_STEPS; s++) {
        uint seed = frame * 16u + s * 4u + 101u;
        vec3 tried = p + Ir77ViewNormal3(f.id, seed) * reach;

        Ir77Sample there = Sample(tried, f.params);
        if (here.weight <= 0.0 || Ir77Random(f.id, seed + 7777u) * here.weight < there.weight) {
            p = tried;
            here = there;
        }
    }

    bool hidden = Ir77ViewCut(p, f.params);

    Ir77FieldPoint point;
    point.position = p * scale;
    point.size = hidden ? 0.0 : size * scale;
    point.color = vec4(here.value >= 0.0 ? VIEW_PLUS : VIEW_MINUS, hidden ? 0.0 : alpha);
    point.data = vec4(here.value, here.weight, key, scale);  // x value, y weight, z the function it walks, w the scale it's drawn at

    return point;
}

// ---- surface: the slice -----------------------------------------------------------------------------------------------------------------

Ir77FieldPoint Ir77Slice(Ir77FieldIn f) {
    float height = f.params[3];
    float scale = Ir77ViewScale(f.params);
    int plane = clamp(int(f.params[5] + 0.5), 0, 2);
    float offset = f.params[6];

    // the same function and view as last frame -- every point is where it was
    float key = Ir77ViewKey(f.params);
    if (!f.first && f.prev.data.y == key) return f.prev;

    vec2 uv = f.coord.xy;
    vec3 p = plane == 0 ? vec3(uv.x, uv.y, offset) : plane == 1 ? vec3(uv.x, offset, uv.y) : vec3(offset, uv.y, uv.x);
    vec3 normal = plane == 0 ? vec3(0.0, 0.0, 1.0) : plane == 1 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);

    Ir77Sample s = Sample(p, f.params);

    Ir77FieldPoint point;
    point.position = (p + normal * s.value * height) * scale;
    point.size = 0.0;
    point.color = vec4(s.value >= 0.0 ? VIEW_PLUS : VIEW_MINUS, 1.0);  // shading lit; colormap / contours read data.x
    point.data = vec4(s.value, key, 0.0, 0.0);

    return point;
}

// ---- isosurface: the box ------------------------------------------------------------------------------------------------------------------

Ir77FieldPoint Ir77Iso(Ir77FieldIn f) {
    float level = max(f.params[3], 1e-30);
    float scale = Ir77ViewScale(f.params);

    float key = Ir77ViewKey(f.params);
    if (!f.first && f.prev.data.y == key) return f.prev;

    vec3 p = f.coord;
    Ir77Sample s = Sample(p, f.params);

    // data.x = value / level -- the surface is where |data.x| crosses 1 (field_compute.glsl); cut away: 0 (outside), so the cut is capped
    float v = Ir77ViewCut(p, f.params) ? 0.0 : s.value / level;

    Ir77FieldPoint point;
    point.position = p * scale;
    point.size = 0.0;
    point.color = vec4(s.value >= 0.0 ? VIEW_PLUS : VIEW_MINUS, 1.0);
    point.data = vec4(v, key, s.value, 0.0);  // x the value over the level, y the key, z the value itself

    return point;
}

Ir77FieldPoint Field(Ir77FieldIn f) {
    uint topology = Ir77ViewTopology();
    return topology == 1u ? Ir77Cloud(f) : topology == 2u ? Ir77Iso(f) : Ir77Slice(f);
}

#endif
