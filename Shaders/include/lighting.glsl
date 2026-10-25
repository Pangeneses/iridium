// =============================================================================================================================================
// lighting.glsl -- the scene's lights and shadows, shared by every lit fragment shader (frag.frag, the field fragment shader).
// Needs bindless.glsl first. Ir77Lighting(world, n) = the sum of every light's contribution at a point, shadows included -- the caller adds
// ambient and multiplies by its albedo.
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
// One light's shadow by ray: from the surface toward the light, ending `distance` along it (the light itself, or unbounded for the sun). 1 = lit.
// The ray starts a hair off the surface along the normal -- enough that it can't hit the triangle it leaves, growing with the size of the
// coordinates so float precision far from the origin can't defeat it.
float LightRayShadow(vec3 world, vec3 n, vec3 to_light, float distance) {
    float magnitude = max(max(abs(world.x), abs(world.y)), abs(world.z));
    vec3 origin = world + n * (0.001 + magnitude * 0.00002);

    return Ir77Occluded(origin, to_light, 0.0, distance) ? 0.0 : 1.0;
}
#endif

// Every light at one point: colour * intensity * falloff * N.L * shadow, summed
vec3 Ir77Lighting(vec3 world, vec3 n) {
    vec3 lit = vec3(0.0);

    uint count = IR77_LIGHTS.count;

    for (uint i = 0u; i < count; i++) {
        Ir77Light light = IR77_LIGHTS.lights[i];

        vec3 to_light;
        float attenuation = 1.0;
        float ray_length = 1.0e30;  // how far a shadow ray goes -- unbounded toward the sun, up to the light otherwise

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

        float diffuse = max(dot(n, to_light), 0.0);
        float shadow = 1.0;

        if (diffuse > 0.0 && attenuation > 0.0) {
#ifdef IR77_RAY_SHADOWS
            shadow = (light.shadow >= 0 && IR77_HAS_TLAS) ? LightRayShadow(world, n, to_light, ray_length) : LightShadow(light.shadow, world, n, to_light);
#else
            shadow = LightShadow(light.shadow, world, n, to_light);
#endif
        }

        lit += light.color * light.intensity * attenuation * diffuse * shadow;
    }

    return lit;
}

#endif
