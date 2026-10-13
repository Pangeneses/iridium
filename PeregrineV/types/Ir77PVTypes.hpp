#pragma once

#include <vk_mem_alloc.h>

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// forward declarations -- interfaces held by shared_ptr in the structs below
// =============================================================================================================================================
struct IIr77PVBuffer;
struct IIr77PVPipeline;

// =============================================================================================================================================
// limits / capacities
// =============================================================================================================================================
static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 3;

static constexpr std::size_t MAX_WINDOWS = 8;

// rows in the material table (one Static buffer per device)
static constexpr std::uint32_t MATERIAL_CAPACITY = 256;

static constexpr std::uint32_t SKELETON_CAPACITY = 64;

// meshoptimizer limits -- must match max_vertices / max_primitives in the mesh shader. Triangles must be divisible by 4.
static constexpr std::size_t MESHLET_MAX_VERTICES = 64;

static constexpr std::size_t MESHLET_MAX_TRIANGLES = 124;

// 0 = pure locality, higher = tighter normal cones for backface culling
static constexpr float MESHLET_CONE_WEIGHT = 0.25f;

// =============================================================================================================================================
// bindless -- one layout, one descriptor set per device. Must match Shader/include/bindless.glsl.
// =============================================================================================================================================

// set 0 binding numbers
inline constexpr std::uint32_t BINDLESS_BINDING_TEXTURES = 0;
inline constexpr std::uint32_t BINDLESS_BINDING_BUFFERS = 1;
inline constexpr std::uint32_t BINDLESS_BINDING_STORAGE_IMAGES = 2;
inline constexpr std::uint32_t BINDLESS_BINDING_COUNT = 3;

// requested array sizes -- Ir77PVLayout clamps them to the device's update-after-bind limits
inline constexpr std::uint32_t BINDLESS_TEXTURES = 4096;
inline constexpr std::uint32_t BINDLESS_BUFFERS = 4096;
inline constexpr std::uint32_t BINDLESS_STORAGE_IMAGES = 256;

// texture slot 0 always holds the white placeholder, so an unset texture slot samples something valid
inline constexpr std::uint32_t BINDLESS_PLACEHOLDER_TEXTURE = 0;

// "no slot" for optional buffer / texture slots
inline constexpr std::uint32_t BINDLESS_NO_SLOT = UINT32_MAX;

// array sizes actually in use on this device
typedef struct Ir77PVBindlessCapacity {
    std::uint32_t textures{0};
    std::uint32_t buffers{0};
    std::uint32_t storage_images{0};
}* pIr77PVBindlessCapacity;

// One push-constant block for every pipeline -- graphics, compute and ray. 128 bytes, the guaranteed minimum.
// Must match the Ir77PushConstants block in bindless.glsl byte for byte.
//   Mesh:   vertices = vertices slot, user[0] = meshlets, [1] = meshlet vertex indices, [2] = meshlet triangles, [3] = draw records
//   Shadow: user[0] = light index
//   CEF:    user[0] = overlay texture slot
typedef struct Ir77PVPushConstants {
    std::array<float, 16> model{};        // column-major, like glm::mat4
    std::uint32_t frame{0};               // buffer slot: this frame's frame table
    std::uint32_t instances{0};           // buffer slot: instance table
    std::uint32_t materials{0};           // buffer slot: material table
    std::uint32_t material{0};            // row in the material table
    std::uint32_t vertices{0};            // buffer slot: pulled / mesh / compute vertex data
    std::uint32_t bones{0};               // buffer slot: bone palette
    std::uint32_t draw_base{0};           // first draw / meshlet for this call
    std::uint32_t mode{0};                // shading mode
    std::array<std::uint32_t, 8> user{};  // per-kind data, see above
}* pIr77PVPushConstants;

static_assert(sizeof(Ir77PVPushConstants) == 128, "Ir77PVPushConstants must be exactly 128 bytes");

// Frame table -- one row per window per frame in flight, in its own buffer slot (pc.frame): the slots of that frame's buffers and textures.
// std430, 32 bytes. Must match Ir77FrameTable in bindless.glsl.
typedef struct Ir77PVFrameTable {
    std::uint32_t camera{BINDLESS_NO_SLOT};                  // buffer slot: Ir77PVCamera
    std::uint32_t lights{BINDLESS_NO_SLOT};                  // buffer slot: Ir77PVLightsHeader + Ir77PVLight[]
    std::uint32_t instances{BINDLESS_NO_SLOT};               // buffer slot: mat4[] -- also pc.instances
    std::uint32_t shadow_matrices{BINDLESS_NO_SLOT};         // buffer slot: mat4[]
    std::uint32_t bones{BINDLESS_NO_SLOT};                   // buffer slot: bone bases + palettes -- pc.bones when a draw has none
    std::uint32_t env{BINDLESS_PLACEHOLDER_TEXTURE};         // texture slot: environment / IBL
    std::uint32_t shadow_map{BINDLESS_PLACEHOLDER_TEXTURE};  // texture slot: shadow map
    std::uint32_t reserved{0};
}* pIr77PVFrameTable;

static_assert(sizeof(Ir77PVFrameTable) == 32, "Ir77PVFrameTable must match Ir77FrameTable in bindless.glsl");

// Per window per frame: the table's own slot plus a CPU copy of what it holds (Ir77PVCmdBuffer builds push constants from it)
typedef struct Ir77PVFrameSlots {
    std::uint32_t table{BINDLESS_NO_SLOT};
    Ir77PVFrameTable values{};
}* pIr77PVFrameSlots;

// =============================================================================================================================================
// passes
// =============================================================================================================================================

// Which render pass a draw list is recorded into.
// Shadow      -> depth-only shadow pass (Shadow / ShadowSkinned pipelines)
// Opaque      -> main pass, first (Static / Skinned / Mesh pipelines)
// Transparent -> main pass, after opaque; caller sorts back to front (Transparent pipeline)
enum class Ir77PVPass : std::uint8_t { Shadow, Opaque, Transparent };

// Attachment 0 is always color when a pass has color; depth is the last attachment.
// Main      -- swapchain color + depth, ends PRESENT_SRC (scene + CEF, today)
// Shadow    -- depth only, ends DEPTH_STENCIL_READ_ONLY (sampled later) (shadow maps)
// Offscreen -- color + depth, color ends SHADER_READ_ONLY (HDR scene target, later)
// Post      -- swapchain color only, ends PRESENT_SRC (tonemap / fullscreen passes + CEF, later)
enum class Ir77PVRenderPassKind : std::uint8_t { Main, Shadow, Offscreen, Post };

// =============================================================================================================================================
// pipelines -- every kind uses the one bindless layout; the kind picks shaders, vertex input and raster / blend state
// =============================================================================================================================================

// Static / Skinned / Transparent* -- vertex input, main pass
// Shadow / ShadowSkinned          -- vertex input, depth-only shadow pass
// CEF                             -- fullscreen quad, overlay texture from pc.user[0]
// Mesh                            -- task / mesh / fragment, no vertex input (VK_EXT_mesh_shader only)
enum class Ir77PVPipelineKind : std::uint8_t { Static, Skinned, Shadow, ShadowSkinned, Transparent, CEF, Mesh, SkinnedTransparent, TransparentBack, SkinnedTransparentBack };

// Compute passes -- graphics-adjacent, dispatched before or alongside the render pass
enum class Ir77PVComputeKind : std::uint8_t { Culling, SkinningUpdate, MorphUpdate, ClothSim, ParticleSim, TerrainLOD, GPUOcclusion, IndirectPrep };

// =============================================================================================================================================
// shaders
// =============================================================================================================================================

// Pipeline order within each group. Values are not stable across edits -- don't serialize them.
enum class Ir77PVShaderStage : std::uint8_t {
    // graphics
    Vertex,
    TessellationControl,
    TessellationEvaluation,
    Geometry,
    Fragment,

    // mesh shading (replaces vertex..geometry)
    Task,
    Mesh,

    // compute
    Compute,

    // ray tracing
    RayGeneration,
    RayAnyHit,
    RayClosestHit,
    RayMiss,
    RayIntersection,
    RayCallable,
};

constexpr VkShaderStageFlagBits ToVkStage(Ir77PVShaderStage const stage) {
    switch (stage) {
        case Ir77PVShaderStage::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case Ir77PVShaderStage::TessellationControl:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case Ir77PVShaderStage::TessellationEvaluation:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case Ir77PVShaderStage::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case Ir77PVShaderStage::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case Ir77PVShaderStage::Task:
            return VK_SHADER_STAGE_TASK_BIT_EXT;
        case Ir77PVShaderStage::Mesh:
            return VK_SHADER_STAGE_MESH_BIT_EXT;
        case Ir77PVShaderStage::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        case Ir77PVShaderStage::RayGeneration:
            return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case Ir77PVShaderStage::RayAnyHit:
            return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case Ir77PVShaderStage::RayClosestHit:
            return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        case Ir77PVShaderStage::RayMiss:
            return VK_SHADER_STAGE_MISS_BIT_KHR;
        case Ir77PVShaderStage::RayIntersection:
            return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
        case Ir77PVShaderStage::RayCallable:
            return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    }

    return VK_SHADER_STAGE_VERTEX_BIT;
}

typedef struct Ir77PVShaderInfo {
    Ir77PVShaderStage stage{Ir77PVShaderStage::Vertex};
    VkPipelineShaderStageCreateInfo stage_create_info{};
    std::vector<char> byte_code{};
    std::uint32_t size{0};
}* pIr77PVShaderInfo;

// One shader file the loader looks for; optional ones are skipped when missing
typedef struct Ir77PVShaderFile {
    std::string file{};
    std::uint64_t id{0};
    Ir77PVShaderStage stage{Ir77PVShaderStage::Vertex};
    bool required{false};
    bool mesh_shading{false};  // only loaded on a device with VK_EXT_mesh_shader enabled
}* pIr77PVShaderFile;

// =============================================================================================================================================
// buffers / textures
// =============================================================================================================================================

// Dynamic: host-mapped, one copy per frame in flight, written with Update() every frame
// Static:  device-local, single copy, written once through a Staging buffer
// Staging: host-mapped TRANSFER_SRC, one upload batch (UploadBatch)
enum class Ir77PVBufferMode : std::uint8_t { Dynamic, Static, Staging };

// Frame-level buffers every window owns, one Dynamic copy per frame in flight; every copy has a bindless slot listed in the frame table.
// Mesh vertex/index buffers are per-asset and live in m_buffers_vertex / m_buffers_index (filled by Ir77PVAsset).
enum class Ir77PVBufferSlot : std::uint8_t { Camera, Lights, Instances, ShadowMatrices, Bones, Indirect };

// Color -- albedo / emissive: sRGB, mipmapped, sampled
// Data  -- normal / ORM / AO / masks: UNORM (no gamma), mipmapped, sampled
// Cube  -- env / IBL: 6 layers, sRGB by default (SetFormat for HDR), mipmapped, sampled
// Depth -- shadow map: D32, depth attachment + sampled, no mips, no upload
enum class Ir77PVTextureKind : std::uint8_t { Color, Data, Cube, Depth };

// Default textures for texture slots that have no real texture yet. White is always texture slot 0.
enum class Ir77PVPlaceholderKind : std::uint8_t { White, Normal };

typedef struct Ir77PVFrameSizes {
    VkDeviceSize camera{256};                       // view, proj, view_proj, eye position
    VkDeviceSize lights{16 + 64 * 64};              // 16-byte header + 64 lights x 64 bytes
    VkDeviceSize instances{8192 * 64};              // 8192 instances x mat4
    VkDeviceSize shadow_matrices{4 * 64};           // 4 cascades / lights x mat4
    VkDeviceSize bones{8192 * 4 + 256 * 128 * 64};  // bone_base per instance slot + 256 skeletons x 128 joints x mat4
    VkDeviceSize indirect{8192 * sizeof(VkDrawIndexedIndirectCommand)};
}* pIr77PVFrameSizes;

// One destination buffer + the data going into it, for a batched multi-buffer upload
// (one staging buffer, one command buffer, one submit covering several entries at once).
typedef struct Ir77PVUploadEntry {
    VkBuffer dst{VK_NULL_HANDLE};
    void const* data{nullptr};
    VkDeviceSize size{0};
    VkDeviceSize dst_offset{0};  // where in dst this entry lands -- 0 = start of the buffer
}* pIr77PVUploadEntry;

typedef struct Ir77PVSamplerDesc {
    VkFilter filter{VK_FILTER_LINEAR};
    VkSamplerAddressMode address{VK_SAMPLER_ADDRESS_MODE_REPEAT};
    float anisotropy{0.0f};  // 0 = off; >0 requires samplerAnisotropy enabled on the device
    bool compare{false};     // depth compare (sampler2DShadow / PCF); Depth kind only
}* pIr77PVSamplerDesc;

inline constexpr std::uint32_t IR77PV_NO_TEXTURE = UINT32_MAX;

// Texture ids (Ir77PVAsset::CreateTexture) for a material's maps. IR77PV_NO_TEXTURE = placeholder (flat normal for the normal map).
typedef struct Ir77PVMaterialTextures {
    std::uint32_t albedo{IR77PV_NO_TEXTURE};
    std::uint32_t normal{IR77PV_NO_TEXTURE};
    std::uint32_t orm{IR77PV_NO_TEXTURE};
    std::uint32_t emissive{IR77PV_NO_TEXTURE};
    std::uint32_t ao{IR77PV_NO_TEXTURE};
}* pIr77PVMaterialTextures;

// =============================================================================================================================================
// vertex formats -- CPU-side input. Ir77PVVertex is also read as a raw float[8] stream by the mesh shader (pc.vertices).
// =============================================================================================================================================
typedef struct Ir77PVVertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
}* pIr77PVVertex;

static_assert(sizeof(Ir77PVVertex) == sizeof(float) * 8, "mesh shader reads vertices as 8 tightly packed floats");

typedef struct Ir77PVVertexSkinned {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
    glm::uvec4 joints{};
    glm::vec4 weights{};
}* pIr77PVVertexSkinned;

// =============================================================================================================================================
// shader-visible data -- std430, layouts must match the GLSL structs
// =============================================================================================================================================

// frame table .camera:  struct Camera { mat4 view; mat4 proj; mat4 view_proj; vec4 eye; };
typedef struct Ir77PVCamera {
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::mat4 view_proj{1.0f};
    glm::vec4 eye{0.0f};
}* pIr77PVCamera;

typedef struct Ir77PVLight {
    glm::vec3 position{};
    float _pad0{};
    glm::vec3 direction{};
    float _pad1{};
    glm::vec3 color{};
    float intensity{};
    float range{};
    float innerConeAngle{};
    float outerConeAngle{};
    int type{};
}* pIr77PVLight;

static_assert(sizeof(Ir77PVLight) == 64, "lights buffer is read as 64-byte std430 structs");

// frame table .lights header -- the light count, padded to 16 bytes; the Ir77PVLight array follows it
typedef struct Ir77PVLightsHeader {
    std::uint32_t count{0};
    std::uint32_t _pad[3]{};
}* pIr77PVLightsHeader;

static_assert(sizeof(Ir77PVLightsHeader) == 16, "lights header must stay 16 bytes so the array stays aligned");

// Material constants -- the head of each material-table row. vec4, vec4, then 4 floats; must stay a 16-byte multiple.
typedef struct Ir77PVMaterialConstants {
    std::array<float, 4> base_color{1.0f, 1.0f, 1.0f, 1.0f};
    std::array<float, 4> emissive{0.0f, 0.0f, 0.0f, 0.0f};
    float roughness{1.0f};
    float metallic{0.0f};
    float alpha_cutoff{0.5f};
    float normal_scale{1.0f};
}* pIr77PVMaterialConstants;

static_assert(sizeof(Ir77PVMaterialConstants) % 16 == 0, "Ir77PVMaterialConstants must be a 16-byte multiple for the std430 material table");

// One row of the material table (pc.materials, row pc.material). Must match Ir77MaterialRow in bindless.glsl.
typedef struct Ir77PVMaterialRow {
    Ir77PVMaterialConstants constants{};
    std::array<std::uint32_t, 4> textures0{};  // texture slots: albedo, normal, ORM, emissive
    std::array<std::uint32_t, 4> textures1{};  // texture slots: AO, -, -, -
}* pIr77PVMaterialRow;

static_assert(sizeof(Ir77PVMaterialRow) == 80, "Ir77PVMaterialRow must match Ir77MaterialRow in bindless.glsl");

// Meshlet record (mesh path, pc.user[0]). std430: vec4, vec4, uvec4.
typedef struct Ir77PVMeshlet {
    std::array<float, 4> sphere{};  // xyz center, w radius -- object space
    std::array<float, 4> cone{};    // xyz axis, w cutoff
    std::uint32_t vertex_offset{0};
    std::uint32_t triangle_offset{0};
    std::uint32_t vertex_count{0};
    std::uint32_t triangle_count{0};
}* pIr77PVMeshlet;

static_assert(sizeof(Ir77PVMeshlet) == 48, "must match the std430 Meshlet struct in the task / mesh shaders");

// Draw record (mesh path, pc.user[3]). std430: uvec4. draws[draw_base + gl_DrawID] says which object a draw is.
typedef struct Ir77PVMeshDraw {
    std::uint32_t instance{0};  // index into the instance table
    std::uint32_t meshlet_offset{0};
    std::uint32_t meshlet_count{0};
    std::uint32_t _pad{0};
}* pIr77PVMeshDraw;

static_assert(sizeof(Ir77PVMeshDraw) == 16, "must match MeshDraw in mesh.task");

// Indirect command record -- same layout as VkDrawIndexedIndirectCommand
typedef struct Ir77MeshRange {
    std::uint32_t index_count{0};
    std::uint32_t instance_count{0};
    std::uint32_t index_offset{0};
    std::int32_t vertex_offset{0};
    std::uint32_t first_instance{0};
}* pIr77MeshRange;

static_assert(sizeof(Ir77MeshRange) == sizeof(VkDrawIndexedIndirectCommand));

// =============================================================================================================================================
// asset handles -- what Ir77PVAsset hands back after an upload
// =============================================================================================================================================

// Whole-scene vertex-domain upload. Scene (glTF loader) stacks every primitive's data into these
// arrays itself -- one entry per vertex/index across the entire file, not per mesh -- and prebuilds
// indirect_commands with the offsets it already knows from that stacking. Asset does not compute
// per-mesh ranges or hand back CPU-side mesh IDs; GPU-side indexing comes entirely from
// indirect_commands at draw time via vkCmdDrawIndexedIndirect.
// Materials, textures and animation are separate channels -- not stacked into this struct.
typedef struct Ir77PVSceneUpload {
    std::vector<Ir77PVVertex> vertices{};
    std::vector<std::uint32_t> indices{};
    std::vector<Ir77PVVertexSkinned> vertices_skinned{};
    std::vector<std::uint32_t> indices_skinned{};
    std::vector<VkDrawIndexedIndirectCommand> indirect_commands{};
}* pIr77PVSceneUpload;

typedef struct Ir77PVInputBuffer {
    std::vector<Ir77PVVertex> vertex_data{};
    std::vector<std::uint32_t> index_data{};
}* pIr77PVInputBuffer;

// Uploaded mesh: device-local vertex + index buffers (single-mesh path, used by the testbed and any
// one-off load). Scene loading uses UploadScene instead -- one shared buffer per channel, not
// one buffer pair per mesh.
typedef struct Ir77PVMesh {
    std::shared_ptr<IIr77PVBuffer> vertex{};
    std::shared_ptr<IIr77PVBuffer> index{};
    std::uint32_t index_count{0};
    VkIndexType index_type{VK_INDEX_TYPE_UINT32};
}* pIr77PVMesh;

// A material is a row in the material table
typedef struct Ir77PVMaterial {
    std::uint32_t row{0};
}* pIr77PVMaterial;

// Meshlet buffers and their bindless slots. Slot order matches Ir77PVDrawItem::meshlet_slots:
//   [0] meshlets   [1] meshlet vertex indices   [2] meshlet triangles   [3] vertices   [4] draw records
inline constexpr std::size_t MESHLET_SLOT_COUNT = 5;

typedef struct Ir77PVMeshletMesh {
    std::shared_ptr<IIr77PVBuffer> meshlets{};
    std::shared_ptr<IIr77PVBuffer> meshlet_vertices{};
    std::shared_ptr<IIr77PVBuffer> meshlet_triangles{};
    std::shared_ptr<IIr77PVBuffer> vertices{};
    std::shared_ptr<IIr77PVBuffer> draws{};
    std::shared_ptr<IIr77PVBuffer> indirect{};
    std::array<std::uint32_t, MESHLET_SLOT_COUNT> slots{BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT};
    std::uint32_t meshlet_count{0};
}* pIr77PVMeshletMesh;

// vertex path scene -- one buffer per channel for every stacked object
typedef struct Ir77PVSceneSlot {
    std::shared_ptr<IIr77PVBuffer> vertex{};
    std::shared_ptr<IIr77PVBuffer> index{};
    std::shared_ptr<IIr77PVBuffer> vertex_skinned{};
    std::shared_ptr<IIr77PVBuffer> index_skinned{};
    std::shared_ptr<IIr77PVBuffer> indirect{};             // Dynamic -- rewritten by Tick when dirty
    std::vector<VkDrawIndexedIndirectCommand> commands{};  // live values -- instanceCount 0 = hidden
    std::vector<std::uint32_t> instance_counts{};          // what a visible command draws
    std::vector<std::uint8_t> visible{};
    std::uint32_t dirty{0};  // frame copies still to rewrite
    bool alive{false};
}* pIr77PVSceneSlot;

// mesh path scene -- meshlet buffers plus the per-record command state
typedef struct Ir77PVMeshletSlot {
    Ir77PVMeshletMesh mesh{};
    std::vector<VkDrawMeshTasksIndirectCommandEXT> commands{};  // live, one per record -- groupCountX 0 = hidden
    std::vector<std::uint32_t> groups{};                        // what a visible record dispatches
    std::vector<std::uint32_t> first_record{};                  // per scene command, plus one end entry
    std::uint32_t dirty{0};
    bool alive{false};
}* pIr77PVMeshletSlot;

// Unloaded buffers -- held until every frame that could still read them has finished. Their bindless slots are released at unload
// (the table delays slot reuse on its own).
typedef struct Ir77PVGrave {
    std::vector<std::shared_ptr<IIr77PVBuffer>> buffers{};
    std::uint32_t frames_left{0};
}* pIr77PVGrave;

// =============================================================================================================================================
// draw
// =============================================================================================================================================

// One draw in a pass list. Everything a shader reads beyond vertex input comes through push constants built from these fields
// (Ir77PVCmdBuffer). Raster kinds use vertex / index; Mesh uses meshlet_slots and meshlet_offset / meshlet_count.
typedef struct Ir77PVDrawItem {
    std::shared_ptr<IIr77PVPipeline> pipeline{};

    std::uint32_t material{0};              // row in the material table -> pc.material
    std::uint32_t bones{BINDLESS_NO_SLOT};  // buffer slot of this draw's bone palette; NO_SLOT = the frame's bones buffer -> pc.bones
    std::uint32_t mode{0};                  // shading mode -> pc.mode

    // raster kinds
    std::shared_ptr<IIr77PVBuffer> vertex{};
    std::shared_ptr<IIr77PVBuffer> index{};
    VkIndexType index_type{VK_INDEX_TYPE_UINT32};
    std::uint32_t index_count{0};
    std::uint32_t first_index{0};
    std::int32_t vertex_offset{0};
    std::uint32_t instance_count{1};
    std::uint32_t first_instance{0};

    // Mesh -> pc.vertices = meshlet_slots[3], pc.user[0..3] = meshlet_slots[0], [1], [2], [4]
    std::array<std::uint32_t, MESHLET_SLOT_COUNT> meshlet_slots{BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT};
    std::uint32_t meshlet_offset{0};  // first meshlet, pushed to the task shader
    std::uint32_t meshlet_count{0};   // task groups = ceil(meshlet_count / task workgroup size)
    std::uint32_t draw_base{0};       // -> pc.draw_base

    // indirect -- VkDrawIndexedIndirectCommand, or VkDrawMeshTasksIndirectCommandEXT for Mesh
    std::shared_ptr<IIr77PVBuffer> indirect{};
    VkDeviceSize indirect_offset{0};
    std::uint32_t indirect_count{0};

    std::array<float, 16> model{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};  // -> pc.model
    std::uint32_t light_index{0};  // shadow kinds -> pc.user[0]
}* pIr77PVDrawItem;

// =============================================================================================================================================
// ray tracing
// =============================================================================================================================================

// SBT region a group's handle is written into
enum class Ir77PVRayRegion : std::uint8_t { RayGen, Miss, Hit, Callable };

// One shader group. Indices refer to Ir77PVPipelineRay::SetStages.
// General (raygen / miss / callable) sets `general`; triangle hit groups set closest_hit / any_hit;
// procedural hit groups also set intersection.
typedef struct Ir77PVRayGroup {
    Ir77PVRayRegion region{Ir77PVRayRegion::RayGen};
    VkRayTracingShaderGroupTypeKHR type{VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR};
    std::uint32_t general{VK_SHADER_UNUSED_KHR};
    std::uint32_t closest_hit{VK_SHADER_UNUSED_KHR};
    std::uint32_t any_hit{VK_SHADER_UNUSED_KHR};
    std::uint32_t intersection{VK_SHADER_UNUSED_KHR};
}* pIr77PVRayGroup;

}  // namespace NSIr77PeregrineV