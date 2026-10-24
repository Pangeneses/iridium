// =============================================================================================================================================
// field_vertex.glsl -- builds one field vertex from the point buffer (no vertex input). Needs bindless.glsl and field.glsl.
//   pc.user[4] = config slot, pc.user[5] = points slot, pc.mode = 0 surface / 1 particles
//   surface:   (dims.x - 1) * (dims.y - 1) cells, 6 vertices each -- the z = 0 slice of the grid; normal from the neighbours
//   particles: one camera-facing quad per element, 6 vertices each, half size = point.size
// =============================================================================================================================================
#ifndef IR77_FIELD_VERTEX_GLSL
#define IR77_FIELD_VERTEX_GLSL

#define IR77_FIELD_CONFIG ir77_field_configs[pc.user[4]].data[0]
#define IR77_FIELD_POINT(i) ir77_field_points[pc.user[5]].data[i]

struct Ir77FieldVertex {
    vec3 world;
    vec3 normal;
    vec4 color;
    float value;
    vec2 coord;   // surface: the domain coordinate (canvas / Paint); particles: unused
    vec2 corner;  // particles: -1..1 across the quad; surface: unused
    vec4 ink;     // surface: point ink at this vertex (pen target "points"); otherwise 0
};

Ir77FieldVertex Ir77BuildFieldVertex(uint vertex) {
    Ir77FieldConfig cfg = IR77_FIELD_CONFIG;
    Ir77FieldVertex v;

    uint corner_index = vertex % 6u;

    if (pc.mode == 1u) {
        // particles
        const vec2 corners[6] = vec2[6](vec2(-1, -1), vec2(1, -1), vec2(1, 1), vec2(-1, -1), vec2(1, 1), vec2(-1, 1));

        Ir77FieldPoint p = IR77_FIELD_POINT(vertex / 6u);
        mat4 view = IR77_CAMERA.view;

        vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 up = vec3(view[0][1], view[1][1], view[2][1]);

        v.corner = corners[corner_index];
        v.world = p.position + (right * v.corner.x + up * v.corner.y) * p.size;
        v.normal = vec3(view[0][2], view[1][2], view[2][2]);  // towards the camera
        v.color = p.color;
        v.value = p.data.x;
        v.coord = vec2(0.0);
        v.ink = vec4(0.0);
        return v;
    }

    // surface -- cell corners (0,0) (1,0) (1,1) / (0,0) (1,1) (0,1)
    const uvec2 offsets[6] = uvec2[6](uvec2(0, 0), uvec2(1, 0), uvec2(1, 1), uvec2(0, 0), uvec2(1, 1), uvec2(0, 1));

    uint dx = cfg.dims.x;
    uint dy = cfg.dims.y;
    uint cell = vertex / 6u;

    uvec2 g = uvec2(cell % (dx - 1u), cell / (dx - 1u)) + offsets[corner_index];
    uint i = g.x + g.y * dx;

    Ir77FieldPoint p = IR77_FIELD_POINT(i);

    // normal from the neighbours' positions (one-sided at the edges)
    uint xl = g.x > 0u ? i - 1u : i, xr = g.x + 1u < dx ? i + 1u : i;
    uint yl = g.y > 0u ? i - dx : i, yr = g.y + 1u < dy ? i + dx : i;

    vec3 along_x = IR77_FIELD_POINT(xr).position - IR77_FIELD_POINT(xl).position;
    vec3 along_y = IR77_FIELD_POINT(yr).position - IR77_FIELD_POINT(yl).position;

    vec2 grid = vec2(g) / vec2(max(dx - 1u, 1u), max(dy - 1u, 1u));

    v.world = p.position;
    v.normal = normalize(cross(along_y, along_x));  // flat sheet in X/Z -> +Y
    v.color = p.color;
    v.value = p.data.x;
    v.coord = vec2(mix(cfg.domain_x.x, cfg.domain_x.y, grid.x), mix(cfg.domain_x.z, cfg.domain_x.w, grid.y));
    v.corner = vec2(0.0);
    v.ink = cfg.ink.y == IR77_FIELD_TARGET_POINTS ? ir77_field_inks[cfg.ink.x].data[i] : vec4(0.0);
    return v;
}

#endif
