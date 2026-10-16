#version 460
#extension GL_GOOGLE_include_directive : require

#include "include/bindless.glsl"

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// set per pipeline: on for opaque (double-sided petals), off for the transparent kinds
layout(constant_id = 0) const bool FLIP_BACK_FACES = true;
const float EXPOSURE = 1.0;
const float AMBIENT = 0.03;

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

void main() {
    Ir77MaterialRow material = IR77_MATERIAL;

    // culling is off -- light back faces from the side being looked at
    vec3 n = normalize(fragNormal);
    if (FLIP_BACK_FACES && !gl_FrontFacing) n = -n;

    // textures0.x = albedo slot (white placeholder when the material has none)
    vec4 base = material.base_color * texture(IR77_TEXTURE(material.textures0.x), fragUV);
    if (base.a < material.alpha_cutoff) discard;
    vec3 albedo = base.rgb;

    vec3 lit = vec3(0.0);

    uint count = IR77_LIGHTS.count;

    for (uint i = 0u; i < count; i++) {
        Ir77Light light = IR77_LIGHTS.lights[i];

        vec3 to_light;
        float attenuation = 1.0;

        if (light.type == 0) {
            to_light = -normalize(light.direction);
        } else {
            vec3 offset = light.position - fragWorldPos;
            float distance = length(offset);
            to_light = offset / max(distance, 0.0001);
            attenuation = DistanceFalloff(distance, light.range);

            if (light.type == 2) attenuation *= SpotFalloff(light, to_light);
        }

        float diffuse = max(dot(n, to_light), 0.0);
        lit += light.color * light.intensity * attenuation * diffuse;
    }

    vec3 color = albedo * (AMBIENT + lit * EXPOSURE);

    // Reinhard -- keeps bright physical intensities from clipping to white
    color = color / (1.0 + color);

    outColor = vec4(color, base.a);
}
