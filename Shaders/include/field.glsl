// =============================================================================================================================================
// field.glsl -- the field contract: what a field program reads and returns, and the helpers it may use. Needs bindless.glsl first, and
// one stage define: IR77_STAGE_COMPUTE (field.comp), IR77_STAGE_VERTEX (field.vert / field_shadow.vert) or IR77_STAGE_FRAGMENT (field.frag).
//
// A program is GLSL source the engine compiles twice -- into field.comp and field.frag -- with this header in front:
//   Ir77FieldPoint Field(Ir77FieldIn f)            required  -- one element: where it is and what it carries (compute, every frame)
//   vec4 Paint(vec2 coord, float t, float p[8])    optional  -- curves drawn on a surface per pixel; rgb colour, a coverage (fragment)
//   vec2 Pen(uint pen, float t, float p[8])        optional  -- the pen(s): spirograph x,y / oscilloscope y (compute). They trace into the canvas
//                                                              (a picture over the surface) or into point ink (the surface grid's own points)
// Every function is compiled into both stages; each stage only calls its own, the others are dead code.
// =============================================================================================================================================
#ifndef IR77_FIELD_GLSL
#define IR77_FIELD_GLSL

// One element -- std430, 48 bytes. Must match Ir77FieldPointGPU in Ir77Animate.hpp.
struct Ir77FieldPoint {
    vec3 position;  // world position (surface: usually vec3(coord.x, height, coord.y) -- Y up)
    float size;     // particle half size, world units; ignored on a surface
    vec4 color;     // straight colour; .a = opacity (particles)
    vec4 data;      // .x = value for the colour map / contours; .yzw = free state that persists (velocity ...)
};

// What the engine hands Field -- one element
struct Ir77FieldIn {
    uvec3 id;              // element index
    uvec3 dims;            // element counts; dims.z == 1 -> 2D
    vec3 coord;            // id mapped into the domain ranges
    float time;            // seconds
    float dt;              // seconds since last frame
    bool first;            // first frame after a compile / reset -- seed state here
    Ir77FieldPoint prev;   // this element's output last frame
    vec4 ink;              // point ink at this element (pen target "points"): rgb colour, a amount; 0 otherwise
    float params[8];       // live values from the UI (field.params)
};

// The field's settings -- one row in its own storage buffer, written on compile and on field.params. Must match Ir77FieldConfig in
// Ir77Animate.hpp (336 bytes).
struct Ir77FieldConfig {
    vec4 domain_x;     // x, y = coord.x range; z, w = coord.y range
    vec4 domain_z;     // x, y = coord.z range; z, w = colour map value range (value at z -> 0, at w -> 1)
    uvec4 dims;        // xyz element counts, w topology (0 surface, 1 particles)
    uvec4 shading;     // x mode (0 lit colour, 1 colour map, 2 colour map + contours), y colour map, z has Paint, w has canvas
    vec4 contour;      // x spacing (value units), y width (pixels)
    uvec4 canvas;      // x storage image slot, y texture slot, z size (pixels, square), w pen count
    vec4 trace;        // x pen speed (t per second), y decay (per second), z sweep rate (domain widths per unit t; 0 = time-bound), w unused
    uvec4 steps;       // x pen samples per frame
    vec4 params[2];    // params[0..7]
    vec4 pen_color[8];
    vec4 pen_width[2];  // pens 0..7 -- canvas: pixels; point ink: grid cells
    uvec4 ink;          // x point-ink buffer slot, y pen target (0 canvas, 1 points)
};

IR77_BUFFER(ir77_field_configs, Ir77FieldConfig);

// point ink -- one vec4 per element (rgb colour, a amount), the pens' target when it is "points"
#define IR77_FIELD_TARGET_CANVAS 0u
#define IR77_FIELD_TARGET_POINTS 1u

#ifdef IR77_STAGE_COMPUTE
IR77_BUFFER_RW(ir77_field_points, Ir77FieldPoint);
IR77_BUFFER_RW(ir77_field_inks, vec4);
#else
IR77_BUFFER(ir77_field_points, Ir77FieldPoint);
IR77_BUFFER(ir77_field_inks, vec4);
#endif

// ---- helpers -------------------------------------------------------------------------------------------------------------------------------

// pcg3d -- a stable 32-bit hash of an element (and a seed)
uvec3 Ir77Pcg3d(uvec3 v) {
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    return v;
}

uint Ir77Hash(uvec3 id) { return Ir77Pcg3d(id).x; }

// stable random in [0, 1) per element and seed -- the same every frame
float Ir77Random(uvec3 id, uint seed) { return float(Ir77Pcg3d(id + uvec3(seed * 747796405u, seed * 2891336453u, seed)).x >> 8) / 16777216.0; }

// Coverage of a line at `distance` pixels from its centre, `width` pixels wide -- anti-aliased over one pixel
float Ir77Line(float distance, float width) { return 1.0 - smoothstep(width * 0.5 - 0.5, width * 0.5 + 0.5, distance); }

#ifdef IR77_STAGE_FRAGMENT
// Distance in pixels from this pixel to the curve y = f(x), slope-corrected so the line keeps its width on steep parts.
// Works for implicit curves too: Ir77Curve(f(x, y), 0.0) for f(x, y) = 0.
float Ir77Curve(float y, float fx) {
    float g = y - fx;
    return abs(g) / max(length(vec2(dFdx(g), dFdy(g))), 1e-6);
}
#else
// outside a fragment shader there are no pixels -- distance in domain units (Paint is never called here)
float Ir77Curve(float y, float fx) { return abs(y - fx); }
#endif

#endif
