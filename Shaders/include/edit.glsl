// =============================================================================================================================================
// edit.glsl -- edit mode's cage: the model's selected faces, edges and vertex points drawn over it (edit_faces.vert / edit_edges.vert /
// edit_points.vert / edit.frag). Needs bindless.glsl first. Must match Ir77EditPoint in Scene/types/Ir77Types.hpp.
//
//   pc.user[4] = points   (Ir77EditPoint[] -- one per edit vertex per instance, in its instance's space with its models[] slot)
//   pc.user[5] = elements (edges: uvec2[] of points; faces: uvec4[] of points)
//   pc.user[6] = flags    (uint[] per edit vertex -- EDIT_SELECTED)
//   pc.user[7] = mode     (EDIT_MODE_*)
//
// The selection is vertices: an edge shows selected when both ends are, a face when all three corners are. Procedural, no vertex buffer: six
// vertices per point (a camera-facing square) and per edge (a screen-space strip), three per face. Sizes are in clip units of the view's height,
// so they hold on screen at any distance. Depth-tested against the model with a small reversed-Z bias, so the visible side shows and the cage
// never fights the surface under it.
// =============================================================================================================================================
#ifndef IR77_EDIT_GLSL
#define IR77_EDIT_GLSL

struct Ir77EditPoint {
    vec3 position;  // the instance's own space
    uint slot;      // models[] slot -- IR77_MODEL(slot) places it, so the cage follows the model's transform
    uint vertex;    // its edit vertex -- the flag it reads
    uint pad0;
    uint pad1;
    uint pad2;
};

IR77_BUFFER(ir77_edit_points, Ir77EditPoint);
IR77_BUFFER(ir77_edit_edges, uvec2);
IR77_BUFFER(ir77_edit_faces, uvec4);
IR77_BUFFER(ir77_edit_flags, uint);

#define IR77_EDIT_POINTS ir77_edit_points[nonuniformEXT(pc.user[4])]
#define IR77_EDIT_EDGES ir77_edit_edges[nonuniformEXT(pc.user[5])]
#define IR77_EDIT_FACES ir77_edit_faces[nonuniformEXT(pc.user[5])]
#define IR77_EDIT_FLAGS ir77_edit_flags[nonuniformEXT(pc.user[6])]
#define IR77_EDIT_MODE pc.user[7]

#define EDIT_MODE_VERTEX 0u
#define EDIT_MODE_EDGE 1u
#define EDIT_MODE_FACE 2u

#define EDIT_SELECTED 1u

const float EDIT_POINT_HALF = 0.0065;   // half a point's side, clip units of the view's height (~4 px at 1300 px tall)
const float EDIT_EDGE_HALF = 0.0012;    // half an edge's width (~1.5 px wide)
const float EDIT_DEPTH_BIAS = 0.0015;   // reversed-Z: z *= 1 + bias pulls the cage 0.15% of its depth toward the camera

const vec4 EDIT_POINT_COLOR = vec4(0.02, 0.02, 0.02, 1.0);
const vec4 EDIT_EDGE_COLOR = vec4(0.02, 0.02, 0.02, 0.9);
const vec4 EDIT_SELECTED_COLOR = vec4(1.0, 0.55, 0.1, 1.0);  // orange -- points and edges
const vec4 EDIT_FACE_COLOR = vec4(1.0, 0.55, 0.1, 0.22);    // a selected face's tint

// a point into clip space -- placed by its instance's world matrix
vec4 Ir77EditClip(Ir77EditPoint p) { return IR77_CAMERA.view_proj * (IR77_MODEL(p.slot) * vec4(p.position, 1.0)); }

bool Ir77EditSelected(Ir77EditPoint p) { return (IR77_EDIT_FLAGS.data[p.vertex] & EDIT_SELECTED) != 0u; }

// width / height of the view -- clip x runs over the width, y over the height; proj[1][1] is flipped for Vulkan's Y-down
float Ir77EditAspect() { return abs(IR77_CAMERA.proj[1][1]) / max(abs(IR77_CAMERA.proj[0][0]), 1e-8); }

// the bias, applied last
vec4 Ir77EditBias(vec4 clip) { return vec4(clip.xy, clip.z * (1.0 + EDIT_DEPTH_BIAS), clip.w); }

// behind the eye, or not drawn -- a vertex no triangle survives clipping with
const vec4 EDIT_CULLED = vec4(2.0, 2.0, 2.0, 1.0);

// the six corners of a quad as (along, across) in 0..1 x -1..1 -- two triangles, (0,-1) (1,-1) (1,1) / (0,-1) (1,1) (0,1)
vec2 Ir77EditCorner(uint corner) {
    const vec2 corners[6] = vec2[6](vec2(0.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(0.0, -1.0), vec2(1.0, 1.0), vec2(0.0, 1.0));
    return corners[corner];
}

#endif
