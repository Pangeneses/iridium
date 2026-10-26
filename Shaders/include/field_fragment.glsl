// =============================================================================================================================================
// field_fragment.glsl -- main() of field.frag, included after the program. Inputs from field.vert; config from pc.user[4].
//   particles (pc.mode 1): round soft sprite, colour or colour map, unlit (additive pipeline)
//   surface   (pc.mode 0): lit colour or colour map [+ contours], then point ink, then the canvas, then Paint() curves on top -- overlays
//                          are unlit
// =============================================================================================================================================
#ifndef IR77_FIELD_FRAGMENT_GLSL
#define IR77_FIELD_FRAGMENT_GLSL

#include "lighting.glsl"

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec2 fragCoord;
layout(location = 4) in float fragValue;
layout(location = 5) in vec2 fragCorner;
layout(location = 6) in vec4 fragInk;

layout(location = 0) out vec4 outColor;

const float IR77_FIELD_AMBIENT = 0.03;

// sRGB -> linear: the colour maps are defined in sRGB, the swapchain encodes on write
vec3 Ir77ToLinear(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }

// matplotlib viridis, polynomial fit
vec3 Ir77Viridis(float t) {
    const vec3 c0 = vec3(0.2777273272234177, 0.005407344544966578, 0.3340998053353061);
    const vec3 c1 = vec3(0.1050930431085774, 1.404613529898575, 1.384590162594685);
    const vec3 c2 = vec3(-0.3308618287255563, 0.214847559468213, 0.09509516302823659);
    const vec3 c3 = vec3(-4.634230498983486, -5.799100973351585, -19.33244095627987);
    const vec3 c4 = vec3(6.228269936347081, 14.17993336680509, 56.69055260068105);
    const vec3 c5 = vec3(4.776384997670288, -13.74514537774601, -65.35303263337234);
    const vec3 c6 = vec3(-5.435455855934631, 4.645852612178535, 26.3124352495832);
    return c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * (c5 + t * c6)))));
}

// Google Turbo, polynomial fit
vec3 Ir77Turbo(float t) {
    const vec4 r4 = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
    const vec4 g4 = vec4(0.09140261, 2.19418839, 4.84296658, -14.18503333);
    const vec4 b4 = vec4(0.10667330, 12.64194608, -60.58204836, 110.36276771);
    const vec2 r2 = vec2(-152.94239396, 59.28637943);
    const vec2 g2 = vec2(4.27729857, 2.82956604);
    const vec2 b2 = vec2(-89.90310912, 27.34824973);

    vec4 v4 = vec4(1.0, t, t * t, t * t * t);
    vec2 v2 = v4.zw * v4.z;
    return vec3(dot(v4, r4) + dot(v2, r2), dot(v4, g4) + dot(v2, g2), dot(v4, b4) + dot(v2, b2));
}

// 0 viridis, 1 turbo, 2 grey
vec3 Ir77Colormap(uint map, float t) {
    t = clamp(t, 0.0, 1.0);
    vec3 c = map == 1u ? Ir77Turbo(t) : map == 2u ? vec3(t) : Ir77Viridis(t);
    return Ir77ToLinear(clamp(c, 0.0, 1.0));
}

void main() {
    Ir77FieldConfig cfg = ir77_field_configs[pc.user[4]].data[0];

    float range = cfg.domain_z.w - cfg.domain_z.z;
    float t = abs(range) > 1e-12 ? (fragValue - cfg.domain_z.z) / range : 0.0;

    if (pc.mode == 1u) {
        // particles -- additive, so order doesn't matter; soft round edge
        float r = length(fragCorner);
        if (r > 1.0) discard;

        vec3 rgb = cfg.shading.x == 0u ? fragColor.rgb : Ir77Colormap(cfg.shading.y, t);
        outColor = vec4(rgb, fragColor.a * (1.0 - smoothstep(0.4, 1.0, r)));
        return;
    }

    // surface -- one lit side, the side the normal points out of; the underside gets ambient only. Shadows: rays on a ray-query device
    // (CompileField defines IR77_RAY_SHADOWS), the shadow map otherwise.
    vec3 n = normalize(fragNormal);

    vec3 albedo = cfg.shading.x == 0u ? fragColor.rgb : Ir77Colormap(cfg.shading.y, t);

    if (cfg.shading.x == 2u && cfg.contour.x > 0.0) {
        // iso lines every `spacing` value units, `width` pixels wide whatever the slope
        float v = fragValue / cfg.contour.x;
        float d = abs(fract(v - 0.5) - 0.5) / max(fwidth(v), 1e-6);
        albedo = mix(albedo, albedo * 0.15, Ir77Line(d, cfg.contour.y));
    }

    // ambient on both sides, occluded from the side being looked at; direct light on the lit side only
    float occlusion = Ir77AmbientOcclusion(fragWorldPos, gl_FrontFacing ? n : -n, gl_FrontFacing ? n : -n);
    vec3 color = albedo * (IR77_FIELD_AMBIENT * occlusion + (gl_FrontFacing ? Ir77Lighting(fragWorldPos, n) : vec3(0.0)));
    color = color / (1.0 + color);

    float params[8] = float[8](cfg.params[0].x, cfg.params[0].y, cfg.params[0].z, cfg.params[0].w, cfg.params[1].x, cfg.params[1].y,
                               cfg.params[1].z, cfg.params[1].w);

    // point ink -- the pens' trace in the grid's own points, blended across each triangle
    color = mix(color, fragInk.rgb, clamp(fragInk.a, 0.0, 1.0));

    // canvas -- the pens' trace, laid over the domain
    if (cfg.shading.w != 0u) {
        vec2 uv = (fragCoord - vec2(cfg.domain_x.x, cfg.domain_x.z)) / vec2(cfg.domain_x.y - cfg.domain_x.x, cfg.domain_x.w - cfg.domain_x.z);
        vec4 c = texture(IR77_TEXTURE(cfg.canvas.y), uv);
        color = mix(color, c.rgb, c.a);
    }

#ifdef IR77_HAS_PAINT
    vec4 p = Paint(fragCoord, IR77_CAMERA.time.x, params);
    color = mix(color, p.rgb, clamp(p.a, 0.0, 1.0));
#endif

    outColor = vec4(color, 1.0);
}

#endif
