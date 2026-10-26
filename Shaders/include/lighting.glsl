// =============================================================================================================================================
// lighting.glsl -- the scene's lights and shadows, shared by every lit fragment shader (frag.frag, the field fragment shader).
// Needs bindless.glsl first.
//   Ir77Shade(surface)   metallic-roughness PBR (glTF): every light's GGX specular + diffuse at a point, shadows included -- frag.frag
//   Ir77Lighting(world, n)  diffuse only, the sum of every light's N.L at a point -- the caller adds ambient and multiplies by its albedo (the field)
//   Ir77AmbientOcclusion(world, n, ng)  how open a point is to the sky, 0..1 -- ray-traced (RTAO) on a ray-query device, 1 otherwise. Scales the
//                                       ambient term only; direct light has its own shadows.
//
// Units: a light's colour * intensity is the radiance that lights a white diffuse surface facing it to that value -- the diffuse BRDF carries no
// 1 / PI and the specular one is scaled by PI to match, so both stay in the units the lights already have.
//
// Shadows: a light casts when it has a shadow tile (light.shadow >= 0). Built with IR77_RAY_SHADOWS (frag_ray.frag, field.frag on a ray-query
// device) a caster's shadow is a ray against the frame's TLAS -- exact, no acne, no resolution limit -- and the shadow map only stands in while
// there is no TLAS. Without it, the shadow map as always.
// =============================================================================================================================================
#ifndef IR77_LIGHTING_GLSL
#define IR77_LIGHTING_GLSL

#ifdef IR77_RAY_SHADOWS
#include "raytrace.glsl"
#endif

const float IR77_PI = 3.14159265359;

// glTF KHR_lights_punctual range falloff: smooth to zero at `range`, inverse square inside it
float DistanceFalloff(float distance, float range) {
    float inv_square = 1.0 / max(distance * distance, 0.0001);
    if (range <= 0.0) return inv_square;

    float ratio = distance / range;
    float window = clamp(1.0 - ratio * ratio * ratio * ratio, 0.0, 1.0);
    return window * window * inv_square;
}

// glTF spot cone: full inside inner, smooth to zero at outer
float SpotFalloff(Ir77Light light, vec3 to_light) {
    float cos_outer = cos(light.outer_cone);
    float cos_inner = cos(light.inner_cone);
    float scale = 1.0 / max(cos_inner - cos_outer, 0.001);
    float cd = dot(normalize(light.direction), -to_light);
    float t = clamp((cd - cos_outer) * scale, 0.0, 1.0);
    return t * t;
}

// One light's shadow from its atlas tile: 3x3 taps on the compare sampler, each one 2x2 hardware PCF. 1 = lit.
// Orthographic (sun) and perspective (spot) both work -- clip.w is 1 for the first, view depth for the second.
float LightShadow(int tile, vec3 world, vec3 n, vec3 to_light) {
    uint slot = IR77_FRAME.shadow_map;
    if (tile < 0 || slot == IR77_PLACEHOLDER_TEXTURE) return 1.0;  // not a caster, or no shadow map (the placeholder has no compare sampler)

    mat4 light = IR77_SHADOW_MATRIX(tile);

    float atlas = float(textureSize(IR77_SHADOW_TEXTURE(slot), 0).x);
    float tile_size = atlas / float(IR77_SHADOW_ATLAS_SIDE);

    // one tile texel in world units at this point: 2 * w / (x scale * tile size) -- w = 1 orthographic, view depth perspective
    float w = (light * vec4(world, 1.0)).w;
    if (w <= 0.0) return 1.0;  // behind a spot

    float texel_world = 2.0 * w / (length(vec3(light[0][0], light[1][0], light[2][0])) * tile_size);

    // normal offset -- more at grazing angles, where acne starts
    float grazing = 1.0 - clamp(dot(n, to_light), 0.0, 1.0);
    vec4 clip = light * vec4(world + n * texel_world * (0.5 + 1.5 * grazing), 1.0);

    vec3 ndc = clip.xyz / clip.w;
    if (ndc.z >= 1.0 || any(greaterThan(abs(ndc.xy), vec2(1.0)))) return 1.0;  // outside this light's frustum

    vec2 local = ndc.xy * 0.5 + 0.5;  // 0..1 inside the tile -- the matrix already flips Y

    // tile -> atlas; taps clamped half a texel inside the tile so the PCF never reads a neighbour
    vec2 origin = vec2(float(uint(tile) % IR77_SHADOW_ATLAS_SIDE), float(uint(tile) / IR77_SHADOW_ATLAS_SIDE)) / float(IR77_SHADOW_ATLAS_SIDE);
    float scale = 1.0 / float(IR77_SHADOW_ATLAS_SIDE);
    vec2 texel = vec2(1.0 / atlas);
    vec2 low = origin + texel * 0.5;
    vec2 high = origin + vec2(scale) - texel * 0.5;

    vec2 uv = origin + local * scale;
    float depth = ndc.z - 0.0002;  // a hair of constant bias; the normal offset does the real work

    float sum = 0.0;
    for (int y = -1; y <= 1; y++)
        for (int x = -1; x <= 1; x++) sum += texture(IR77_SHADOW_TEXTURE(slot), vec3(clamp(uv + vec2(x, y) * texel, low, high), depth));

    return sum / 9.0;
}

#ifdef IR77_RAY_SHADOWS
// Where a ray leaving the surface starts: a hair off it along the geometric normal -- enough that it can't hit the triangle it leaves, growing
// with the size of the coordinates so float precision far from the origin can't defeat it
vec3 Ir77RayOrigin(vec3 world, vec3 ng) {
    float magnitude = max(max(abs(world.x), abs(world.y)), abs(world.z));
    return world + ng * (0.001 + magnitude * 0.00002);
}

// One light's shadow by ray: from the surface toward the light, ending `distance` along it (the light itself, or unbounded for the sun). 1 = lit.
float LightRayShadow(vec3 world, vec3 n, vec3 to_light, float distance) {
    return Ir77Occluded(Ir77RayOrigin(world, n), to_light, 0.0, distance) ? 0.0 : 1.0;
}

// ---- ray-traced ambient occlusion -----------------------------------------------------------------------------------------------------------

const uint IR77_RTAO_RAYS = 4u;      // per pixel -- noisy per pixel, smooth across a few
const float IR77_RTAO_RADIUS = 0.5;  // world units -- geometry further away than this doesn't occlude

// Per-pixel rotation of the ray pattern -- interleaved gradient noise, fixed per pixel (no temporal accumulation to average a moving one)
float Ir77InterleavedGradientNoise(vec2 pixel) { return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715)))); }

// IR77_RTAO_RAYS short rays over the hemisphere around n, cosine-weighted (golden-angle spiral, rotated per pixel); the fraction that escape
// within IR77_RTAO_RADIUS. Alpha-tested geometry counts only where it's solid (Ir77Occluded). No TLAS -> 1.
float Ir77AmbientOcclusion(vec3 world, vec3 n, vec3 ng) {
    if (!IR77_HAS_TLAS) return 1.0;

    vec3 t = normalize(cross(n, abs(n.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));
    vec3 b = cross(n, t);

    vec3 origin = Ir77RayOrigin(world, ng);
    float rotation = Ir77InterleavedGradientNoise(gl_FragCoord.xy) * 2.0 * IR77_PI;

    float open = 0.0;

    for (uint k = 0u; k < IR77_RTAO_RAYS; k++) {
        float u = (float(k) + 0.5) / float(IR77_RTAO_RAYS);
        float r = sqrt(u);  // cosine-weighted: more rays near the normal, where occlusion matters most
        float phi = float(k) * 2.39996323 + rotation;

        vec3 direction = t * (r * cos(phi)) + b * (r * sin(phi)) + n * sqrt(1.0 - u);

        // a normal map can tilt a ray below the real surface -- mirror it back above
        float below = dot(direction, ng);
        if (below < 0.0) direction -= 2.0 * below * ng;

        open += Ir77Occluded(origin, direction, 0.0, IR77_RTAO_RADIUS) ? 0.0 : 1.0;
    }

    return open / float(IR77_RTAO_RAYS);
}
#else
float Ir77AmbientOcclusion(vec3 world, vec3 n, vec3 ng) { return 1.0; }
#endif

// One light arriving at a point: the direction to it and the radiance that gets there (colour * intensity * falloff * shadow). false = nothing
// arrives (out of range, outside the cone, or the surface faces away). ng = the geometric normal -- the shadow lookups offset along it, and a
// surface facing away from the light is shadowed whatever its shading normal says.
bool Ir77LightArrival(Ir77Light light, vec3 world, vec3 ng, out vec3 to_light, out vec3 radiance) {
    float attenuation = 1.0;
    float ray_length = 1.0e30;  // how far a shadow ray goes -- unbounded toward the sun, up to the light otherwise

    radiance = vec3(0.0);

    if (light.type == 0) {
        to_light = -normalize(light.direction);
    } else {
        vec3 offset = light.position - world;
        float distance = length(offset);
        to_light = offset / max(distance, 0.0001);
        attenuation = DistanceFalloff(distance, light.range);
        ray_length = max(distance - 0.01, 0.0);  // stop short of the light -- a lamp's own bulb mesh doesn't shadow it

        if (light.type == 2) attenuation *= SpotFalloff(light, to_light);
    }

    if (attenuation <= 0.0 || dot(ng, to_light) <= 0.0) return false;

#ifdef IR77_RAY_SHADOWS
    float shadow = (light.shadow >= 0 && IR77_HAS_TLAS) ? LightRayShadow(world, ng, to_light, ray_length) : LightShadow(light.shadow, world, ng, to_light);
#else
    float shadow = LightShadow(light.shadow, world, ng, to_light);
#endif

    radiance = light.color * light.intensity * attenuation * shadow;

    return shadow > 0.0;
}

// Every light at one point: colour * intensity * falloff * N.L * shadow, summed -- diffuse only, no albedo
vec3 Ir77Lighting(vec3 world, vec3 n) {
    vec3 lit = vec3(0.0);

    for (uint i = 0u; i < IR77_LIGHTS.count; i++) {
        vec3 to_light;
        vec3 radiance;
        if (!Ir77LightArrival(IR77_LIGHTS.lights[i], world, n, to_light, radiance)) continue;

        lit += radiance * max(dot(n, to_light), 0.0);
    }

    return lit;
}

// ---- metallic-roughness PBR ------------------------------------------------------------------------------------------------------------------

// One shaded point -- what Ir77Shade needs
struct Ir77Surface {
    vec3 world;      // position
    vec3 n;          // shading normal (normal-mapped), unit, facing the viewer's side
    vec3 ng;         // geometric normal, unit, the same side -- shadow offsets
    vec3 v;          // to the eye, unit
    vec3 albedo;     // base colour, linear
    float metallic;  // 0 dielectric .. 1 metal
    float roughness; // perceptual, 0 mirror .. 1 rough
};

// Specular colour at normal incidence: 4% for dielectrics, the base colour for metals
vec3 Ir77F0(Ir77Surface s) { return mix(vec3(0.04), s.albedo, s.metallic); }

// GGX / Trowbridge-Reitz distribution. a2 = roughness^4 (alpha = perceptual roughness squared).
float Ir77DistributionGGX(float nh, float a2) {
    float d = nh * nh * (a2 - 1.0) + 1.0;
    return a2 / (IR77_PI * d * d);
}

// Height-correlated Smith visibility -- G / (4 N.L N.V) in one term
float Ir77VisibilitySmith(float nl, float nv, float a2) {
    float l = nv * sqrt(nl * nl * (1.0 - a2) + a2);
    float v = nl * sqrt(nv * nv * (1.0 - a2) + a2);
    return 0.5 / max(l + v, 1e-6);
}

vec3 Ir77FresnelSchlick(float vh, vec3 f0) { return f0 + (1.0 - f0) * pow(1.0 - vh, 5.0); }

// Every light's diffuse + specular at one point, shadows included -- the lit colour, before ambient and emissive
vec3 Ir77Shade(Ir77Surface s) {
    float roughness = clamp(s.roughness, 0.045, 1.0);  // a perfect mirror has no highlight to see -- keep a pinpoint
    float alpha = roughness * roughness;
    float a2 = alpha * alpha;

    vec3 f0 = Ir77F0(s);
    float nv = max(dot(s.n, s.v), 1e-4);

    vec3 lit = vec3(0.0);

    for (uint i = 0u; i < IR77_LIGHTS.count; i++) {
        vec3 to_light;
        vec3 radiance;
        if (!Ir77LightArrival(IR77_LIGHTS.lights[i], s.world, s.ng, to_light, radiance)) continue;

        float nl = dot(s.n, to_light);
        if (nl <= 0.0) continue;  // the normal map turned this pixel away

        vec3 h = normalize(to_light + s.v);
        float nh = max(dot(s.n, h), 0.0);
        float vh = max(dot(s.v, h), 0.0);

        vec3 f = Ir77FresnelSchlick(vh, f0);
        vec3 specular = Ir77DistributionGGX(nh, a2) * Ir77VisibilitySmith(nl, nv, a2) * f;
        vec3 diffuse = (1.0 - f) * (1.0 - s.metallic) * s.albedo;  // what isn't reflected goes in; metals have no diffuse

        lit += (diffuse + IR77_PI * specular) * radiance * nl;
    }

    return lit;
}

#endif
