// =============================================================================================================================================
// outline.glsl -- the selection's outline (outline.vert / outline_skinned.vert / outline.mesh / outline.frag). Needs bindless.glsl first.
//
//   pc.user[4] = flags  (OUTLINE_KEEP_BACK_MESHLETS: mesh.task keeps the back-facing meshlets; OUTLINE_MASK: the mask draw, below)
//   pc.user[5] = colour (RGBA8, packUnorm4x8 -- Ir77PVFrameAsset::outline)
//
// An inverted hull: the model drawn again with its front faces culled, every vertex pushed out along its normal by a fixed width on screen,
// in one flat colour. What shows past the model's edge is the outline; the model's own surface hides the rest. Front faces must stay culled:
// pushed sideways on screen they keep their own depth, which lands in front of the surface they cover. Depth-tested, so it hides behind
// what's in front. A hard edge (split normals) opens a small gap at its corner; an open sheet (a petal, a leaf) has no far side to outline.
//
// Transparent surfaces write no depth, so their shell would show straight through them: they're drawn once more first as a mask -- in place,
// depth only, both sides (outline.frag writes alpha 0 through an alpha-blend pipeline) -- after every transparent draw, then the hull.
// =============================================================================================================================================
#ifndef IR77_OUTLINE_GLSL
#define IR77_OUTLINE_GLSL

#define OUTLINE_KEEP_BACK_MESHLETS 1u
#define OUTLINE_MASK 2u

#define IR77_OUTLINE_FLAGS pc.user[4]
#define IR77_OUTLINE_COLOR pc.user[5]

const float OUTLINE_WIDTH = 0.0035;  // clip units of the view's height (~2.3 px at 1300 px tall)

bool Ir77OutlineMask() { return (IR77_OUTLINE_FLAGS & OUTLINE_MASK) != 0u; }

// a clip-space position pushed out along its world normal -- the normal's direction on screen, the same pixel width at any distance
vec4 Ir77OutlineExtrude(vec4 clip, vec3 world_normal) {
    if (clip.w <= 1e-6) return clip;

    // width / height of the view -- proj[1][1] is flipped for Vulkan's Y-down
    float aspect = abs(IR77_CAMERA.proj[1][1]) / max(abs(IR77_CAMERA.proj[0][0]), 1e-8);

    // the normal on screen, in units of the height both ways -- then back to clip x / y
    vec2 dir = (IR77_CAMERA.view_proj * vec4(world_normal, 0.0)).xy * vec2(aspect, 1.0);
    float len = length(dir);
    if (len < 1e-8) return clip;

    clip.xy += (dir / len) * vec2(1.0 / aspect, 1.0) * OUTLINE_WIDTH * 2.0 * clip.w;
    return clip;
}

// a vertex of the draw -- the mask in place; the hull pushed out
vec4 Ir77OutlinePlace(vec4 clip, vec3 world_normal) {
    return Ir77OutlineMask() ? clip : Ir77OutlineExtrude(clip, world_normal);
}

#endif
