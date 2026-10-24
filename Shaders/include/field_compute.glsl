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

    if (cfg.trace.z > 0.0) p.x = cfg.domain_x.x + fract(t * cfg.trace.z) * (cfg.domain_x.y - cfg.domain_x.x);

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

        if (to_points)
            Ir77StampSegmentPoints(cfg, a, b, cfg.pen_color[pen], cfg.pen_width[pen / 4u][pen % 4u]);
        else
            Ir77StampSegment(cfg, a, b, cfg.pen_color[pen], cfg.pen_width[pen / 4u][pen % 4u]);
    }
}

#endif
