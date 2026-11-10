// =============================================================================================================================================
// field_compute.glsl -- main() of field.comp, included after the program. One pipeline, three jobs by pc.user[2]:
//   0 points  -- Field() per element: reads its last output, writes the new one
//   1 canvas  -- decay (or clear) every canvas pixel -- or every element's point ink, when the pens target the points
//   2 pen     -- Pen() sampled steps times per pen between t_from and t_to, each step stamped as an anti-aliased segment into the canvas
//                (width in pixels) or into the point ink of the surface grid (width in grid cells)
// push: user[0] config slot, user[1] points slot, user[2] job, user[3] flags (1 first, 2 clear canvas), user[4] / user[5] t_from / t_to (float
// bits). 64 threads per group; groups spread over x and y so a 2048^2 canvas fits the 65535 group limit.
// =============================================================================================================================================
#ifndef IR77_FIELD_COMPUTE_GLSL
#define IR77_FIELD_COMPUTE_GLSL

layout(local_size_x = 64) in;

IR77_STORAGE_IMAGE(ir77_rgba8_images, rgba8);

#define IR77_FIELD_JOB_POINTS 0u
#define IR77_FIELD_JOB_CANVAS 1u
#define IR77_FIELD_JOB_PEN 2u
#define IR77_FIELD_JOB_ISO_CLEAR 3u
#define IR77_FIELD_JOB_ISO 4u
#define IR77_FIELD_JOB_MARKERS 5u

// ---- markers -- the program's Marker() in world units (include/view.glsl defines its own, scaled, before this) --------------------------
#if defined(IR77_HAS_MARKERS) && !defined(IR77_FIELD_MARKER_WORLD)
#define IR77_FIELD_MARKER_WORLD
vec4 Ir77FieldMarkerWorld(uint i, float p[8], out vec4 color) { return Marker(i, p, color); }
#endif

// ---- isosurface: marching tetrahedra ----------------------------------------------------------------------------------------------------
// Each grid cell (a cube of 8 points, corner k at +x for bit 0, +y bit 1, +z bit 2) is cut into 6 tetrahedra round its 0-7 diagonal (they
// match across neighbouring cells, so the surface has no cracks). A tetrahedron with some corners inside (|data.x| >= 1) and some outside
// gives one triangle (1 or 3 inside) or two (2 inside), corners where the value crosses 1 along its edges. Positions and normals are
// interpolated there; colour and value are the inside corner's.
// Both work in log|value| where they can: a field that falls off exponentially (a density, an orbital's tail) is a straight line in log, so
// where the edge crosses 1 and which way the gradient points come out right at grid spacing -- interpolating |value| itself puts the
// surface off by a fraction of a cell, differently cell to cell (a sphere comes out bumpy). Near zero (a node) |value| is used as it is.

// below this (|value| over the level) the log isn't used -- a node, where log|value| dives to minus infinity
#define IR77_ISO_LOG_FLOOR 1e-3

const uvec4 IR77_ISO_TETS[6] = uvec4[6](uvec4(0, 1, 3, 7), uvec4(0, 1, 5, 7), uvec4(0, 2, 3, 7), uvec4(0, 2, 6, 7), uvec4(0, 4, 5, 7), uvec4(0, 4, 6, 7));

struct Ir77IsoCorner {
    vec3 position;
    vec3 normal;  // outward -- down the gradient of |value|
    vec4 color;
    float value;  // signed
    float level;  // |value|
};

uint Ir77IsoIndex(uvec3 g, uvec3 dims) { return g.x + g.y * dims.x + g.z * dims.x * dims.y; }

float Ir77IsoLevel(uvec3 g, uvec3 dims) { return abs(ir77_field_points[pc.user[1]].data[Ir77IsoIndex(g, dims)].data.x); }

Ir77IsoCorner Ir77IsoCornerAt(uvec3 g, uvec3 dims) {
    Ir77FieldPoint p = ir77_field_points[pc.user[1]].data[Ir77IsoIndex(g, dims)];

    Ir77IsoCorner c;
    c.position = p.position;
    c.color = p.color;
    c.value = p.data.x;
    c.level = abs(p.data.x);

    // the gradient -- central differences across the neighbours (one-sided at the edges), over the distance between them; of log|value|
    // when the corner and all six neighbours are clear of zero (same direction, far more even across the grid), else of |value|
    bool logs = c.level > IR77_ISO_LOG_FLOOR;
    for (int axis = 0; axis < 3 && logs; axis++) {
        uvec3 lo = g;
        uvec3 hi = g;
        if (g[axis] > 0u) lo[axis] -= 1u;
        if (g[axis] + 1u < dims[axis]) hi[axis] += 1u;
        logs = Ir77IsoLevel(lo, dims) > IR77_ISO_LOG_FLOOR && Ir77IsoLevel(hi, dims) > IR77_ISO_LOG_FLOOR;
    }

    vec3 gradient;
    for (int axis = 0; axis < 3; axis++) {
        uvec3 lo = g;
        uvec3 hi = g;
        if (g[axis] > 0u) lo[axis] -= 1u;
        if (g[axis] + 1u < dims[axis]) hi[axis] += 1u;

        vec3 plo = ir77_field_points[pc.user[1]].data[Ir77IsoIndex(lo, dims)].position;
        vec3 phi = ir77_field_points[pc.user[1]].data[Ir77IsoIndex(hi, dims)].position;
        float span = max(length(phi - plo), 1e-12);

        float vh = Ir77IsoLevel(hi, dims);
        float vl = Ir77IsoLevel(lo, dims);
        gradient[axis] = (logs ? log(vh) - log(vl) : vh - vl) / span;
    }

    float len = length(gradient);
    c.normal = len > 1e-20 ? -gradient / len : vec3(0.0, 1.0, 0.0);
    return c;
}

// where an edge crosses 1 -- in log|value| when both ends are clear of zero
Ir77FieldIsoVertex Ir77IsoCut(Ir77IsoCorner a, Ir77IsoCorner b) {
    bool logs = a.level > IR77_ISO_LOG_FLOOR && b.level > IR77_ISO_LOG_FLOOR;
    float t = logs ? -log(a.level) / (log(b.level) - log(a.level)) : (1.0 - a.level) / (b.level - a.level);
    t = clamp(t, 0.0, 1.0);

    Ir77FieldIsoVertex v;
    v.position = vec4(mix(a.position, b.position, t), 1.0);
    vec3 n = mix(a.normal, b.normal, t);
    // colour and value from the inside end -- a lobe keeps its own colour right to its edge (the outside end may be another lobe's sign)
    Ir77IsoCorner inside = a.level >= b.level ? a : b;
    v.normal = vec4(length(n) > 1e-12 ? normalize(n) : a.normal, inside.value);
    v.color = inside.color;
    return v;
}

void Ir77IsoEmit(Ir77FieldConfig cfg, Ir77FieldIsoVertex a, Ir77FieldIsoVertex b, Ir77FieldIsoVertex c) {
    uint at = atomicAdd(ir77_field_iso_counters[cfg.iso.y].data[0], 3u);
    if (at + 3u > cfg.iso.z) return;  // over the budget -- dropped

    ir77_field_iso_vertices[cfg.iso.x].data[at] = a;
    ir77_field_iso_vertices[cfg.iso.x].data[at + 1u] = b;
    ir77_field_iso_vertices[cfg.iso.x].data[at + 2u] = c;
}

void Ir77IsoCell(Ir77FieldConfig cfg, uint cell) {
    uvec3 dims = cfg.dims.xyz;
    uvec3 cells = dims - 1u;
    uvec3 g = uvec3(cell % cells.x, (cell / cells.x) % cells.y, cell / (cells.x * cells.y));

    // a quick look first -- most cells are all inside or all outside
    bool any_in = false;
    bool any_out = false;
    for (uint k = 0u; k < 8u; k++) {
        bool inside = Ir77IsoLevel(g + uvec3(k & 1u, (k >> 1) & 1u, (k >> 2) & 1u), dims) >= 1.0;
        any_in = any_in || inside;
        any_out = any_out || !inside;
    }
    if (!any_in || !any_out) return;

    Ir77IsoCorner corners[8];
    for (uint k = 0u; k < 8u; k++) corners[k] = Ir77IsoCornerAt(g + uvec3(k & 1u, (k >> 1) & 1u, (k >> 2) & 1u), dims);

    for (int t = 0; t < 6; t++) {
        uvec4 tet = IR77_ISO_TETS[t];
        uint mask = 0u;
        for (int k = 0; k < 4; k++)
            if (corners[tet[k]].level >= 1.0) mask |= 1u << k;

        uint inside = bitCount(mask);
        if (inside == 0u || inside == 4u) continue;

        if (inside == 1u || inside == 3u) {
            // the lone corner -- the one inside, or the one outside -- and a triangle across its three edges
            uint lone_bit = inside == 1u ? mask : (~mask & 15u);
            int lone = findLSB(lone_bit);
            uint o[3];
            int n = 0;
            for (int k = 0; k < 4; k++)
                if (k != lone) o[n++] = tet[k];

            Ir77IsoCorner l = corners[tet[lone]];
            Ir77IsoEmit(cfg, Ir77IsoCut(l, corners[o[0]]), Ir77IsoCut(l, corners[o[1]]), Ir77IsoCut(l, corners[o[2]]));
        } else {
            // two in (a, b), two out (c, d) -- the quad ac ad bd bc
            uint a = 0u, b = 0u, c = 0u, d = 0u;
            int ni = 0, no = 0;
            for (int k = 0; k < 4; k++) {
                if ((mask & (1u << k)) != 0u) {
                    if (ni++ == 0) a = tet[k];
                    else b = tet[k];
                } else {
                    if (no++ == 0) c = tet[k];
                    else d = tet[k];
                }
            }

            Ir77FieldIsoVertex ac = Ir77IsoCut(corners[a], corners[c]);
            Ir77FieldIsoVertex ad = Ir77IsoCut(corners[a], corners[d]);
            Ir77FieldIsoVertex bd = Ir77IsoCut(corners[b], corners[d]);
            Ir77FieldIsoVertex bc = Ir77IsoCut(corners[b], corners[c]);
            Ir77IsoEmit(cfg, ac, ad, bd);
            Ir77IsoEmit(cfg, ac, bd, bc);
        }
    }
}

float[8] Ir77FieldParams(Ir77FieldConfig cfg) {
    return float[8](cfg.params[0].x, cfg.params[0].y, cfg.params[0].z, cfg.params[0].w, cfg.params[1].x, cfg.params[1].y, cfg.params[1].z,
                    cfg.params[1].w);
}

// a pen sample in domain units -- sweep mode replaces x with the beam's position, which wraps at the right edge
vec2 Ir77PenAt(Ir77FieldConfig cfg, uint pen, float t, float params[8]) {
#ifdef IR77_HAS_PEN
    vec2 p = Pen(pen, t, params);
#else
    vec2 p = vec2(0.0);
#endif

    if (cfg.trace.z > 0.0 && p.x < IR77_PEN_LIFTED) p.x = cfg.domain_x.x + fract(t * cfg.trace.z) * (cfg.domain_x.y - cfg.domain_x.x);

    return p;
}

void Ir77StampSegment(Ir77FieldConfig cfg, vec2 a, vec2 b, vec4 color, float width) {
    float size = float(cfg.canvas.z);
    vec2 span = vec2(cfg.domain_x.y - cfg.domain_x.x, cfg.domain_x.w - cfg.domain_x.z);
    vec2 origin = vec2(cfg.domain_x.x, cfg.domain_x.z);

    // domain -> canvas pixels (same mapping field.frag samples with)
    vec2 pa = (a - origin) / span * size;
    vec2 pb = (b - origin) / span * size;

    float reach = width * 0.5 + 1.0;
    ivec2 low = ivec2(floor(min(pa, pb) - reach));
    ivec2 high = ivec2(ceil(max(pa, pb) + reach));

    low = max(low, ivec2(0));
    high = min(high, ivec2(int(cfg.canvas.z) - 1));
    high = min(high, low + ivec2(63));  // one step never paints more than 64x64 pixels -- raise steps for long fast strokes

    vec2 ab = pb - pa;
    float len2 = max(dot(ab, ab), 1e-8);

    for (int y = low.y; y <= high.y; y++)
        for (int x = low.x; x <= high.x; x++) {
            vec2 p = vec2(x, y) + 0.5;
            float h = clamp(dot(p - pa, ab) / len2, 0.0, 1.0);
            float coverage = Ir77Line(length(p - pa - ab * h), width) * color.a;
            if (coverage <= 0.0) continue;

            ivec2 pixel = ivec2(x, y);
            vec4 old = imageLoad(ir77_rgba8_images[nonuniformEXT(cfg.canvas.x)], pixel);
            imageStore(ir77_rgba8_images[nonuniformEXT(cfg.canvas.x)], pixel, vec4(mix(old.rgb, color.rgb, coverage), max(old.a, coverage)));
        }
}

// One segment into the surface grid's point ink -- domain -> grid coordinates (z = 0 slice), coverage by distance in cells
void Ir77StampSegmentPoints(Ir77FieldConfig cfg, vec2 a, vec2 b, vec4 color, float width) {
    uvec2 dims = cfg.dims.xy;
    vec2 cells = vec2(max(dims, uvec2(2u)) - 1u);
    vec2 span = vec2(cfg.domain_x.y - cfg.domain_x.x, cfg.domain_x.w - cfg.domain_x.z);
    vec2 origin = vec2(cfg.domain_x.x, cfg.domain_x.z);

    vec2 ga = (a - origin) / span * cells;
    vec2 gb = (b - origin) / span * cells;

    float reach = width * 0.5 + 1.0;
    ivec2 low = max(ivec2(floor(min(ga, gb) - reach)), ivec2(0));
    ivec2 high = min(ivec2(ceil(max(ga, gb) + reach)), ivec2(dims) - 1);
    high = min(high, low + ivec2(63));

    vec2 ab = gb - ga;
    float len2 = max(dot(ab, ab), 1e-8);

    for (int y = low.y; y <= high.y; y++)
        for (int x = low.x; x <= high.x; x++) {
            vec2 g = vec2(x, y);
            float h = clamp(dot(g - ga, ab) / len2, 0.0, 1.0);
            float coverage = Ir77Line(length(g - ga - ab * h), width) * color.a;
            if (coverage <= 0.0) continue;

            uint i = uint(x) + uint(y) * dims.x;
            vec4 old = ir77_field_inks[cfg.ink.x].data[i];
            ir77_field_inks[cfg.ink.x].data[i] = vec4(mix(old.rgb, color.rgb, coverage), max(old.a, coverage));
        }
}

void main() {
    Ir77FieldConfig cfg = ir77_field_configs[pc.user[0]].data[0];
    bool to_points = cfg.ink.y == IR77_FIELD_TARGET_POINTS;

    uint i = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * 64u;
    uint job = pc.user[2];
    uint flags = pc.user[3];
    float t_from = uintBitsToFloat(pc.user[4]);
    float t_to = uintBitsToFloat(pc.user[5]);

    float params[8] = Ir77FieldParams(cfg);

    if (job == IR77_FIELD_JOB_POINTS) {
        uvec3 dims = cfg.dims.xyz;
        if (i >= dims.x * dims.y * dims.z) return;

        Ir77FieldIn f;
        f.id = uvec3(i % dims.x, (i / dims.x) % dims.y, i / (dims.x * dims.y));
        f.dims = dims;

        vec3 grid = vec3(f.id) / vec3(max(dims - 1u, uvec3(1u)));
        f.coord = vec3(mix(cfg.domain_x.x, cfg.domain_x.y, grid.x), mix(cfg.domain_x.z, cfg.domain_x.w, grid.y),
                       dims.z > 1u ? mix(cfg.domain_z.x, cfg.domain_z.y, grid.z) : 0.0);
        f.time = IR77_CAMERA.time.x;
        f.dt = IR77_CAMERA.time.y;
        f.first = (flags & 1u) != 0u;
        f.prev = ir77_field_points[pc.user[1]].data[i];
        f.ink = to_points ? ir77_field_inks[cfg.ink.x].data[i] : vec4(0.0);
        f.params = params;

        ir77_field_points[pc.user[1]].data[i] = Field(f);
        return;
    }

    if (job == IR77_FIELD_JOB_MARKERS) {
#ifdef IR77_HAS_MARKERS
        if (i >= min(cfg.markers.y, IR77_FIELD_MAX_MARKERS)) return;

        vec4 color = vec4(1.0);
        vec4 sphere = Ir77FieldMarkerWorld(i, params, color);

        Ir77FieldMarker marker;
        marker.sphere = vec4(sphere.xyz, max(sphere.w, 0.0));
        marker.color = color;
        ir77_field_markers[cfg.markers.x].data[i] = marker;
#endif
        return;
    }

    if (job == IR77_FIELD_JOB_ISO_CLEAR) {
        if (i == 0u) ir77_field_iso_counters[cfg.iso.y].data[0] = 0u;
        return;
    }

    if (job == IR77_FIELD_JOB_ISO) {
        uvec3 cells = max(cfg.dims.xyz, uvec3(2u)) - 1u;
        if (i >= cells.x * cells.y * cells.z) return;

        Ir77IsoCell(cfg, i);
        return;
    }

    if (job == IR77_FIELD_JOB_CANVAS && to_points) {
        uvec3 dims = cfg.dims.xyz;
        if (i >= dims.x * dims.y * dims.z) return;

        ir77_field_inks[cfg.ink.x].data[i] = (flags & 2u) != 0u ? vec4(0.0) : ir77_field_inks[cfg.ink.x].data[i] * max(1.0 - cfg.trace.y * IR77_CAMERA.time.y, 0.0);
        return;
    }

    if (job == IR77_FIELD_JOB_CANVAS) {
        uint size = cfg.canvas.z;
        if (i >= size * size) return;

        ivec2 pixel = ivec2(i % size, i / size);

        vec4 c = (flags & 2u) != 0u ? vec4(0.0) : imageLoad(ir77_rgba8_images[nonuniformEXT(cfg.canvas.x)], pixel) * max(1.0 - cfg.trace.y * IR77_CAMERA.time.y, 0.0);

        imageStore(ir77_rgba8_images[nonuniformEXT(cfg.canvas.x)], pixel, c);
        return;
    }

    if (job == IR77_FIELD_JOB_PEN) {
        uint steps = max(cfg.steps.x, 1u);
        if (i >= steps * cfg.canvas.w) return;

        uint pen = i / steps;
        uint step = i % steps;

        float ta = mix(t_from, t_to, float(step) / float(steps));
        float tb = mix(t_from, t_to, float(step + 1u) / float(steps));

        vec2 a = Ir77PenAt(cfg, pen, ta, params);
        vec2 b = Ir77PenAt(cfg, pen, tb, params);

        if (cfg.trace.z > 0.0 && b.x < a.x) return;  // the beam wrapped between the samples -- no line back across the screen
        if (a.x >= IR77_PEN_LIFTED || b.x >= IR77_PEN_LIFTED) return;  // the pen is up at either end -- nothing between them

        if (to_points)
            Ir77StampSegmentPoints(cfg, a, b, cfg.pen_color[pen], cfg.pen_width[pen / 4u][pen % 4u]);
        else
            Ir77StampSegment(cfg, a, b, cfg.pen_color[pen], cfg.pen_width[pen / 4u][pen % 4u]);
    }
}

#endif
