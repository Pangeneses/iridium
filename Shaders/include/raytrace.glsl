// =============================================================================================================================================
// raytrace.glsl -- ray queries against the frame's TLAS. Include it (after bindless.glsl) only in shaders that trace: it needs GL_EXT_ray_query,
// and the shader registry rejects a shader that uses binding 3 on a device without ray queries.
//
//   IR77_HAS_TLAS                       this frame has a TLAS (IR77_FRAME.tlas set) -- false before the first build or with nothing in it
//   IR77_TLAS                           the frame's TLAS, for rayQueryInitializeEXT
//   Ir77Occluded(origin, dir, t0, t1)   true when geometry lies on origin + dir * [t0, t1] -- shadow / visibility rays. Alpha-tested (MASK)
//                                       geometry only counts where its albedo's alpha passes the cutoff.
//   Ir77RayAlphaPasses(object, prim, b) the alpha test for one candidate hit -- object = the hit's custom index, prim = its triangle,
//                                       b = its barycentrics
//
// The alpha test reads the hit triangle straight out of its object's vertex / index buffers by GPU address (IR77_FRAME.ray_objects names the
// table, Ir77PVRayObject in Ir77PVTypes.hpp), and its material row through pc.materials -- the material table every lit draw carries.
// =============================================================================================================================================
#ifndef IR77_RAYTRACE_GLSL
#define IR77_RAYTRACE_GLSL

#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

#include "bindless.glsl"

// binding 3 -- every TLAS (Acceleration's ring); the frame table says which one is this frame's
layout(set = 0, binding = IR77_BINDING_ACCELERATION) uniform accelerationStructureEXT ir77_acceleration_structures[];

#define IR77_HAS_TLAS (IR77_FRAME.tlas != 0xFFFFFFFFu)
#define IR77_TLAS ir77_acceleration_structures[nonuniformEXT(IR77_FRAME.tlas)]

// One BLAS -- must match Ir77PVRayObject in Ir77PVTypes.hpp. Addresses low / high.
struct Ir77RayObject {
    uvec2 vertex_address;
    uvec2 index_address;
    uint first_index;
    int vertex_offset;
    uint vertex_floats;  // vertex stride in floats -- UV at +6 (Ir77PVVertex: position, normal, uv, tangent)
    uint material;       // material row; 0xFFFFFFFF = opaque
};
IR77_BUFFER(ir77_ray_objects, Ir77RayObject);

// a buffer's contents by GPU address
layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer Ir77RayFloats { float data[]; };
layout(buffer_reference, std430, buffer_reference_align = 4) readonly buffer Ir77RayIndices { uint data[]; };

const uint IR77_RAY_UV_FLOAT = 6u;

// true = this candidate hit counts: the material's alpha at the hit (base colour * albedo texture, mip 0) reaches its cutoff
bool Ir77RayAlphaPasses(uint object, uint primitive, vec2 barycentrics) {
    if (IR77_FRAME.ray_objects == 0xFFFFFFFFu) return true;

    Ir77RayObject o = ir77_ray_objects[IR77_FRAME.ray_objects].data[object];
    if (o.material == 0xFFFFFFFFu) return true;

    Ir77RayIndices indices = Ir77RayIndices(o.index_address);
    Ir77RayFloats vertices = Ir77RayFloats(o.vertex_address);

    uint first = o.first_index + primitive * 3u;
    vec3 weight = vec3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

    vec2 uv = vec2(0.0);
    for (uint k = 0u; k < 3u; k++) {
        uint vertex = uint(int(indices.data[first + k]) + o.vertex_offset);
        uint at = vertex * o.vertex_floats + IR77_RAY_UV_FLOAT;
        uv += weight[k] * vec2(vertices.data[at], vertices.data[at + 1u]);
    }

    Ir77MaterialRow material = ir77_materials[pc.materials].data[o.material];
    float alpha = material.base_color.a * textureLod(IR77_TEXTURE(material.textures0.x), uv, 0.0).a;

    return alpha >= material.alpha_cutoff;
}

// Any hit that counts ends the query -- a shadow ray only needs to know whether something is in the way, not what or where. Opaque
// geometry commits on its own; alpha-tested geometry comes back as candidates, committed only where the test passes.
bool Ir77Occluded(vec3 origin, vec3 direction, float t_min, float t_max) {
    if (!IR77_HAS_TLAS) return false;

    rayQueryEXT query;
    rayQueryInitializeEXT(query, IR77_TLAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFFu, origin, t_min, direction, t_max);

    while (rayQueryProceedEXT(query)) {
        if (rayQueryGetIntersectionTypeEXT(query, false) != gl_RayQueryCandidateIntersectionTriangleEXT) continue;

        uint object = uint(rayQueryGetIntersectionInstanceCustomIndexEXT(query, false));
        uint primitive = uint(rayQueryGetIntersectionPrimitiveIndexEXT(query, false));

        if (Ir77RayAlphaPasses(object, primitive, rayQueryGetIntersectionBarycentricsEXT(query, false))) rayQueryConfirmIntersectionEXT(query);
    }

    return rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT;
}

#endif
