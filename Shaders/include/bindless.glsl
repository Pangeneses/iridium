// =============================================================================================================================================
// bindless.glsl -- the one resource interface every Iridium shader sees. Mirrors Ir77PVLayout + Ir77PVPushConstants.
//
//   #include "include/bindless.glsl"      (compile with: glslc -I Shader ...)
//
//   textures:        IR77_TEXTURE(i)                         sampler2D at slot i
//   shadow maps:     IR77_SHADOW_TEXTURE(i)                  sampler2DShadow at slot i (depth + compare sampler only)
//   storage buffers: IR77_BUFFER(Name, Type) once per type,  then Name[i].data[j]
//   storage images:  IR77_STORAGE_IMAGE(Name, format) once,  then Name[i]
//   rays:            #include "include/raytrace.glsl" too -- IR77_TLAS, Ir77Occluded (ray-query devices only)
//   per draw:        pc.model, pc.frame, pc.material ...     (indices into the arrays above)
// =============================================================================================================================================
#ifndef IR77_BINDLESS_GLSL
#define IR77_BINDLESS_GLSL

#extension GL_EXT_nonuniform_qualifier : require

#define IR77_BINDING_TEXTURES 0
#define IR77_BINDING_BUFFERS 1
#define IR77_BINDING_STORAGE_IMAGES 2
#define IR77_BINDING_ACCELERATION 3  // declared in include/raytrace.glsl -- only shaders that trace rays include it

#define IR77_PLACEHOLDER_TEXTURE 0u

// binding 0 -- every sampled texture
layout(set = 0, binding = IR77_BINDING_TEXTURES) uniform sampler2D ir77_textures[];

// nonuniformEXT makes the index safe when it differs across invocations (per-pixel material, per-meshlet data...)
#define IR77_TEXTURE(i) ir77_textures[nonuniformEXT(i)]

// binding 1 -- every storage buffer. GLSL needs a block type per element type, so declare one alias per type you read:
//   struct Ir77Instance { mat4 model; uint material; uint bones; uint pad0; uint pad1; };
//   IR77_BUFFER(ir77_instances, Ir77Instance);
//   ... ir77_instances[pc.instances].data[gl_InstanceIndex].model
#define IR77_BUFFER(Name, Type) \
    layout(set = 0, binding = IR77_BINDING_BUFFERS, std430) readonly buffer Name##_block { Type data[]; } Name[]

// writable alias of the same binding -- compute / ray outputs
#define IR77_BUFFER_RW(Name, Type) \
    layout(set = 0, binding = IR77_BINDING_BUFFERS, std430) buffer Name##_block { Type data[]; } Name[]

// binding 2 -- storage images, one alias per format
//   IR77_STORAGE_IMAGE(ir77_rgba8_images, rgba8);
//   imageStore(ir77_rgba8_images[nonuniformEXT(pc.user[0])], pixel, colour);
#define IR77_STORAGE_IMAGE(Name, Format) layout(set = 0, binding = IR77_BINDING_STORAGE_IMAGES, Format) uniform image2D Name[]

// Must match Ir77PVPushConstants in Ir77PVTypes.hpp byte for byte -- 128 bytes, every stage
layout(push_constant) uniform Ir77PushConstants {
    mat4 model;      // object transform for direct draws (instanced draws read the instance table instead)
    uint frame;      // buffer slot: this frame's globals (camera, lights, time)
    uint instances;  // buffer slot: instance table
    uint materials;  // buffer slot: material table
    uint material;   // row in the material table
    uint vertices;   // buffer slot: vertex data for pulled / mesh / compute geometry
    uint bones;      // buffer slot: bone palette
    uint draw_base;  // first draw / meshlet for this call
    uint mode;       // shading mode -- lit, height colour map, flat, ...
    uint user[8];    // free per-pass data
}
pc;

// ---- shared tables (Ir77PVTypes.hpp: Ir77PVFrameTable, Ir77PVMaterialRow) -------------------------------------------------------------------

// pc.frame -> this window's frame table for this frame: slots of the camera / lights / ... buffers and of the env / shadow textures
struct Ir77FrameTable {
    uint camera;           // buffer slot
    uint lights;           // buffer slot
    uint instances;        // buffer slot (also pc.instances)
    uint shadow_matrices;  // buffer slot
    uint bones;            // buffer slot (pc.bones when the draw has no palette of its own)
    uint env;              // texture slot
    uint shadow_map;       // texture slot
    uint tlas;             // acceleration-structure slot (binding 3, include/raytrace.glsl): the TLAS built this frame; 0xFFFFFFFF = none
    uint ray_objects;      // buffer slot: Ir77RayObject[] (include/raytrace.glsl) -- row = a hit's custom index; 0xFFFFFFFF = none
    uint commands;         // buffer slot: this frame's copy of Geometry's command pool (VkDrawIndexedIndirectCommand[]) -- what cull.comp reads
};
IR77_BUFFER(ir77_frame_tables, Ir77FrameTable);
#define IR77_FRAME ir77_frame_tables[pc.frame].data[0]

// pc.materials / pc.material -> this draw's material row. textures0 = albedo, normal, ORM, emissive; textures1.x = AO (texture slots)
struct Ir77MaterialRow {
    vec4 base_color;
    vec4 emissive;
    float roughness;
    float metallic;
    float alpha_cutoff;
    float normal_scale;
    uvec4 textures0;
    uvec4 textures1;
};
IR77_BUFFER(ir77_materials, Ir77MaterialRow);
#define IR77_MATERIAL ir77_materials[pc.materials].data[pc.material]

// ---- frame data (Ir77PVTypes.hpp: Ir77PVCamera, Ir77PVLightsHeader + Ir77PVLight, instance mat4s, bone bases + palettes) ---------------------

struct Ir77Camera {
    mat4 view;
    mat4 proj;
    mat4 view_proj;
    vec4 eye;
    vec4 time;   // x seconds, y delta, z frame number
};
IR77_BUFFER(ir77_cameras, Ir77Camera);
#define IR77_CAMERA ir77_cameras[IR77_FRAME.camera].data[0]

// shadow -- binding 0 read again through a compare sampler (only valid on depth textures registered with one: the shadow map)
layout(set = 0, binding = IR77_BINDING_TEXTURES) uniform sampler2DShadow ir77_shadow_textures[];
#define IR77_SHADOW_TEXTURE(i) ir77_shadow_textures[nonuniformEXT(i)]

// one view-projection per shadow-casting light (Ir77Light.shadow) -- pc.user[0] in the Shadow pass
IR77_BUFFER(ir77_shadow_matrices, mat4);

// the shadow map is an atlas of IR77_SHADOW_ATLAS_SIDE x IR77_SHADOW_ATLAS_SIDE tiles, tile i at (i % side, i / side) -- Ir77PVTypes.hpp SHADOW_ATLAS_SIDE
#define IR77_SHADOW_ATLAS_SIDE 2u
#define IR77_SHADOW_MATRIX(i) ir77_shadow_matrices[IR77_FRAME.shadow_matrices].data[i]

// one model matrix per instance slot -- gl_InstanceIndex (vertex path) or the draw record's instance (mesh path)
IR77_BUFFER(ir77_instances, mat4);
#define IR77_MODEL(i) ir77_instances[pc.instances].data[i]

struct Ir77Light {
    vec3 position;
    int shadow;  // shadow atlas tile = IR77_SHADOW_MATRIX index; -1 = casts no shadow
    vec3 direction;  // the way the light points (node -Z)
    float pad1;
    vec3 color;
    float intensity;
    float range;  // 0 = infinite
    float inner_cone;
    float outer_cone;
    int type;  // 0 directional, 1 point, 2 spot
};

layout(set = 0, binding = IR77_BINDING_BUFFERS, std430) readonly buffer Ir77Lights_block {
    uint count;
    uint pad0, pad1, pad2;
    Ir77Light lights[];
}
ir77_lights[];
#define IR77_LIGHTS ir77_lights[IR77_FRAME.lights]

// bone_base[instance] = where that instance's joint palette starts in bones[]
layout(set = 0, binding = IR77_BINDING_BUFFERS, std430) readonly buffer Ir77Bones_block {
    uint bone_base[8192];
    mat4 bones[];
}
ir77_bones[];
#define IR77_BONES ir77_bones[pc.bones]

// Slot conventions per pipeline kind
//   Mesh:   pc.vertices = vertices, pc.user[0] = meshlets, [1] = meshlet vertex indices, [2] = meshlet triangles, [3] = draw records
//   Shadow: pc.user[0] = shadow atlas tile / shadow matrix index
//   CEF:    pc.user[0] = overlay texture slot

#endif
