// =============================================================================================================================================
// field.glsl -- the field contract: what a field program reads and returns, and the helpers it may use. Needs bindless.glsl first, and
// one stage define: IR77_STAGE_COMPUTE (field.comp), IR77_STAGE_VERTEX (field.vert / field_shadow.vert) or IR77_STAGE_FRAGMENT (field.frag).
//
// A program is GLSL source the engine compiles twice -- into field.comp and field.frag -- with this header in front:
//   Ir77FieldPoint Field(Ir77FieldIn f)            required  -- one element: where it is and what it carries (compute, every frame)
//   vec4 Paint(vec2 coord, float t, float p[8])    optional  -- curves drawn on a surface per pixel; rgb colour, a coverage (fragment)
//   vec2 Pen(uint pen, float t, float p[8])        optional  -- the pen(s): spirograph x,y / oscilloscope y (compute). They trace into the canvas
//                                                              (a picture over the surface) or into point ink (the surface grid's own points).
//                                                              Return IR77_PEN_UP to lift the pen: no segment to or from that sample -- strokes
//                                                              apart (a drawing, text)
//   vec4 Marker(uint i, float p[8], out vec4 color) optional -- up to 64 lit spheres (i = 0 .. 63): xyz centre, w radius (0 or less: none).
//                                                              In world units -- or the function's when the program uses include/view.glsl
//                                                              (it applies its scale). Recomputed every frame; drawn under every topology.
//
// Topologies: surface (the grid's z = 0 slice as a sheet), particles (a point per element), isosurface (the 3D grid's points; the surface
// is where |data.x| crosses 1, lit, coloured by the points' colours -- scale data.x by your own level).
//
// Data: field.data uploads an array of floats -- tables, coefficients, anything too big for params[8] (a Hartree-Fock basis and its
// orbitals, say). Any function reads it with Ir77FieldDataCount() / Ir77FieldData(i); it outlives recompiles until replaced or released.
// Every function is compiled into both stages; each stage only calls its own, the others are dead code.
// =============================================================================================================================================
#ifndef IR77_FIELD_GLSL
#define IR77_FIELD_GLSL

// Pen() -- the pen lifted: any sample with x at or past IR77_PEN_LIFTED
#define IR77_PEN_LIFTED 1.0e30
#define IR77_PEN_UP vec2(3.0e30, 0.0)

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
// Fields/types/Ir77Types.hpp (384 bytes).
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
    uvec4 values;       // x data buffer slot (field.data), y how many floats
    uvec4 iso;          // isosurface: x vertex buffer slot, y counter slot, z vertex budget
    uvec4 markers;      // x marker buffer slot, y how many it holds (0: the program has no Marker())
};

// One marker -- std430, 32 bytes. Must match Ir77FieldMarkerGPU in Fields/types/Ir77Types.hpp.
struct Ir77FieldMarker {
    vec4 sphere;  // xyz world centre, w radius (0: none)
    vec4 color;
};

#define IR77_FIELD_MAX_MARKERS 64u
#define IR77_FIELD_MARKER_STACKS 16u
#define IR77_FIELD_MARKER_SLICES 32u

// One isosurface vertex -- std430, 48 bytes. Must match Ir77FieldIsoVertexGPU in Fields/types/Ir77Types.hpp.
struct Ir77FieldIsoVertex {
    vec4 position;  // xyz world
    vec4 normal;    // xyz outward, w the value there (data.x, signed)
    vec4 color;
};

#define IR77_FIELD_TOPOLOGY_ISOSURFACE 2u

IR77_BUFFER(ir77_field_configs, Ir77FieldConfig);

// field.data -- the program's own floats
IR77_BUFFER(ir77_field_data, float);

// the config's slot -- compute pushes it in user[0], the draws in user[4]
#ifdef IR77_STAGE_COMPUTE
#define IR77_FIELD_CONFIG_SLOT pc.user[0]
#else
#define IR77_FIELD_CONFIG_SLOT pc.user[4]
#endif

// point ink -- one vec4 per element (rgb colour, a amount), the pens' target when it is "points"
#define IR77_FIELD_TARGET_CANVAS 0u
#define IR77_FIELD_TARGET_POINTS 1u

#ifdef IR77_STAGE_COMPUTE
IR77_BUFFER_RW(ir77_field_points, Ir77FieldPoint);
IR77_BUFFER_RW(ir77_field_inks, vec4);
IR77_BUFFER_RW(ir77_field_iso_vertices, Ir77FieldIsoVertex);
IR77_BUFFER_RW(ir77_field_iso_counters, uint);
IR77_BUFFER_RW(ir77_field_markers, Ir77FieldMarker);
#else
IR77_BUFFER(ir77_field_points, Ir77FieldPoint);
IR77_BUFFER(ir77_field_inks, vec4);
IR77_BUFFER(ir77_field_iso_vertices, Ir77FieldIsoVertex);
IR77_BUFFER(ir77_field_iso_counters, uint);
IR77_BUFFER(ir77_field_markers, Ir77FieldMarker);
#endif

// One sample of a function -- what a program's Sample() returns for include/view.glsl's generic views. value: the function itself (signed
// is fine -- the slice shows it, its sign colours the cloud); weight: how likely a point is there, never negative (the cloud's density --
// |psi|^2 for an amplitude, the value itself for a density).
struct Ir77Sample {
    float value;
    float weight;
};

// ---- helpers -------------------------------------------------------------------------------------------------------------------------------

// the panel's topology -- 0 surface, 1 particles, 2 isosurface
uint Ir77FieldTopology() { return ir77_field_configs[IR77_FIELD_CONFIG_SLOT].data[0].dims.w; }

// how many floats field.data holds (0: none uploaded)
uint Ir77FieldDataCount() { return ir77_field_configs[IR77_FIELD_CONFIG_SLOT].data[0].values.y; }

// float i of field.data -- 0.0 past the end
float Ir77FieldData(uint i) {
    uvec4 values = ir77_field_configs[IR77_FIELD_CONFIG_SLOT].data[0].values;
    return i < values.y ? ir77_field_data[values.x].data[i] : 0.0;
}

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
