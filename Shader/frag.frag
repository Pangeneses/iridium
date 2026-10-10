#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// set 0 binding 1 -- Ir77PVLightsHeader + Ir77PVLight[] (64 bytes each)
struct Light {
    vec3 position;
    float pad0;
    vec3 direction;   // the way the light points (node -Z)
    float pad1;
    vec3 color;
    float intensity;
    float range;      // 0 = infinite
    float inner_cone;
    float outer_cone;
    int type;         // 0 directional, 1 point, 2 spot
};

layout(std430, set = 0, binding = 1) readonly buffer Lights {
    uint count;
    uint pad0, pad1, pad2;
    Light lights[];
};

// set 2 binding 0 -- Ir77PVMaterialConstants
layout(set = 2, binding = 0) uniform Material {
    vec4 base_color;
    vec4 emissive;
    float roughness;
    float metallic;
    float alpha_cutoff;
    float normal_scale;
} material;

// set 2 binding 1 -- albedo (white placeholder when the material has none)
layout(set = 2, binding = 1) uniform sampler2D albedo_map;

// set per pipeline: on for opaque (double-sided petals), off for the transparent kinds
layout(constant_id = 0) const bool FLIP_BACK_FACES = true;
const float EXPOSURE    = 1.0;
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
float SpotFalloff(Light light, vec3 to_light) {
    float cos_outer = cos(light.outer_cone);
    float cos_inner = cos(light.inner_cone);
    float scale = 1.0 / max(cos_inner - cos_outer, 0.001);
    float cd = dot(normalize(light.direction), -to_light);
    float t = clamp((cd - cos_outer) * scale, 0.0, 1.0);
    return t * t;
}

void main() {
    // culling is off -- light back faces from the side being looked at
    vec3 n = normalize(fragNormal);
    if (FLIP_BACK_FACES && !gl_FrontFacing) n = -n;

    vec4 base = material.base_color * texture(albedo_map, fragUV);
    if (base.a < material.alpha_cutoff) discard;
    vec3 albedo = base.rgb;

    vec3 lit = vec3(0.0);

    for (uint i = 0u; i < count; i++) {
        Light light = lights[i];

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