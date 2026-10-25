#version 460
#extension GL_GOOGLE_include_directive : require

// Built twice by compile_shaders.sh: frag.frag.spv (shadow map), and frag_ray.frag.spv with IR77_RAY_SHADOWS (ray-traced shadows -- the
// manifest's "fragment_ray", used on ray-query devices). One source; lighting.glsl does the difference.

#include "include/bindless.glsl"
#include "include/lighting.glsl"

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) flat in uint fragMaterial;

layout(location = 0) out vec4 outColor;

// set per pipeline: on for opaque (double-sided petals), off for the transparent kinds
layout(constant_id = 0) const bool FLIP_BACK_FACES = true;
const float EXPOSURE = 1.0;
const float AMBIENT = 0.03;

void main() {
    Ir77MaterialRow material = ir77_materials[pc.materials].data[fragMaterial];  // this command's row -- one draw can mix materials

    // culling is off -- light back faces from the side being looked at
    vec3 n = normalize(fragNormal);
    if (FLIP_BACK_FACES && !gl_FrontFacing) n = -n;

    // textures0.x = albedo slot (white placeholder when the material has none)
    vec4 base = material.base_color * texture(IR77_TEXTURE(material.textures0.x), fragUV);
    if (base.a < material.alpha_cutoff) discard;

    vec3 color = base.rgb * (AMBIENT + Ir77Lighting(fragWorldPos, n) * EXPOSURE);

    // Reinhard -- keeps bright physical intensities from clipping to white
    color = color / (1.0 + color);

    outColor = vec4(color, base.a);
}
