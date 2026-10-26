#version 460
#extension GL_GOOGLE_include_directive : require

// Built twice by shader.sh: frag.frag.spv (shadow map), and frag_ray.frag.spv with IR77_RAY_SHADOWS (ray-traced shadows -- the
// manifest's "fragment_ray", used on ray-query devices). One source; lighting.glsl does the difference.
//
// glTF metallic-roughness: base colour (+ alpha test), normal map, metallic / roughness (ORM: G roughness, B metallic), occlusion (R), emissive.
// A missing texture is the white placeholder -- every factor then stands alone, and the normal map is skipped.

#include "include/bindless.glsl"
#include "include/lighting.glsl"

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) flat in uint fragMaterial;
layout(location = 4) in vec4 fragTangent;  // xyz world, w handedness

layout(location = 0) out vec4 outColor;

// set per pipeline: on for opaque (double-sided petals), off for the transparent kinds
layout(constant_id = 0) const bool FLIP_BACK_FACES = true;
const float EXPOSURE = 1.0;
const float AMBIENT = 0.03;  // stands in for the environment until IBL (IR77_FRAME.env)

// The tangent-space normal map onto the surface -- TBN from the interpolated tangent (re-orthogonalised) and its handedness
vec3 Ir77NormalMap(vec3 n, vec4 tangent, uint slot, float scale) {
    if (slot == IR77_PLACEHOLDER_TEXTURE || dot(tangent.xyz, tangent.xyz) < 1e-12) return n;  // no normal map, or no tangents

    vec3 t = normalize(tangent.xyz - n * dot(n, tangent.xyz));
    vec3 b = cross(n, t) * (tangent.w < 0.0 ? -1.0 : 1.0);

    vec3 m = texture(IR77_TEXTURE(slot), fragUV).xyz * 2.0 - 1.0;
    m.xy *= scale;  // glTF normalTexture.scale

    return normalize(mat3(t, b, n) * m);
}

void main() {
    Ir77MaterialRow material = ir77_materials[pc.materials].data[fragMaterial];  // this command's row -- one draw can mix materials

    // textures0 = albedo, normal, ORM, emissive; textures1.x = occlusion
    vec4 base = material.base_color * texture(IR77_TEXTURE(material.textures0.x), fragUV);
    if (base.a < material.alpha_cutoff) discard;

    // normals -- shading from the map, geometric for shadows. Culling is off: a back face is lit from the side being looked at.
    vec3 ng = normalize(fragNormal);
    vec3 n = Ir77NormalMap(ng, fragTangent, material.textures0.y, material.normal_scale);

    if (FLIP_BACK_FACES && !gl_FrontFacing) {
        ng = -ng;
        n = -n;
    }

    vec4 orm = texture(IR77_TEXTURE(material.textures0.z), fragUV);
    // baked occlusion (the texture) times ray-traced (contact with everything else in the scene) -- ambient only
    float occlusion = texture(IR77_TEXTURE(material.textures1.x), fragUV).r * Ir77AmbientOcclusion(fragWorldPos, n, ng);
    vec3 emissive = material.emissive.rgb * texture(IR77_TEXTURE(material.textures0.w), fragUV).rgb;

    Ir77Surface surface;
    surface.world = fragWorldPos;
    surface.n = n;
    surface.ng = ng;
    surface.v = normalize(IR77_CAMERA.eye.xyz - fragWorldPos);
    surface.albedo = base.rgb;
    surface.metallic = clamp(material.metallic * orm.b, 0.0, 1.0);
    surface.roughness = clamp(material.roughness * orm.g, 0.0, 1.0);

    // ambient: diffuse where there is diffuse, the specular colour where there isn't -- occluded; then the lights, then what glows
    vec3 ambient = AMBIENT * occlusion * (surface.albedo * (1.0 - surface.metallic) + Ir77F0(surface));
    vec3 color = ambient + Ir77Shade(surface) * EXPOSURE + emissive;

    // Reinhard -- keeps bright physical intensities from clipping to white
    color = color / (1.0 + color);

    outColor = vec4(color, base.a);
}
