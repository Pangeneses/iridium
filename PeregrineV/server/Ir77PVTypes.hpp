#pragma once

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
struct IIr77PVDescriptorSet;
struct IIr77PVLayout;
struct IIr77PVPipeline;

// =============================================================================================================================================
// limits / capacities
// =============================================================================================================================================
static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 3;

static constexpr std::size_t MAX_WINDOWS = 8;

static constexpr std::uint32_t MATERIAL_CAPACITY = 256;

static constexpr std::uint32_t SKELETON_CAPACITY = 64;

// meshoptimizer limits -- must match max_vertices / max_primitives in the mesh shader. Triangles must be divisible by 4.
static constexpr std::size_t MESHLET_MAX_VERTICES = 64;

static constexpr std::size_t MESHLET_MAX_TRIANGLES = 124;

// 0 = pure locality, higher = tighter normal cones for backface culling
static constexpr float MESHLET_CONE_WEIGHT = 0.25f;

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
// layouts / pipelines
// =============================================================================================================================================

// Set layout per kind
// Static        -- 0 global, 1 pass, 2 material
// Skinned       -- 0 global, 1 pass, 2 material, 3 bones
// Shadow        -- 0 global
// ShadowSkinned -- 0 global, 1 bones
// CEF           -- 0 browser texture
// Mesh          -- 0 global, 1 pass, 2 material, 3 meshlets (VK_EXT_mesh_shader only)
enum class Ir77PVLayoutKind : std::uint8_t { Static, Skinned, Shadow, ShadowSkinned, CEF, Mesh };

// Pipeline kind -> layout kind
// Static        -> Ir77PVLayoutKind::Static
// Skinned       -> Ir77PVLayoutKind::Skinned
// Shadow        -> Ir77PVLayoutKind::Shadow         (depth-only render pass)
// ShadowSkinned -> Ir77PVLayoutKind::ShadowSkinned  (depth-only render pass)
// Transparent   -> Ir77PVLayoutKind::Static
// CEF           -> Ir77PVLayoutKind::CEF
// Mesh          -> Ir77PVLayoutKind::Mesh           (task / mesh / fragment, no vertex input)
enum class Ir77PVPipelineKind : std::uint8_t { Static, Skinned, Shadow, ShadowSkinned, Transparent, Mesh, CEF };

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
// buffers / textures / descriptors
// =============================================================================================================================================

// Dynamic: host-mapped, one copy per frame in flight, written with Update() every frame
// Static:  device-local, single copy, written once with Upload() through a staging buffer
enum class Ir77PVBufferMode : std::uint8_t { Dynamic, Static };

// Frame-level buffers every window owns, one Dynamic copy per frame in flight.
// Mesh vertex/index buffers are per-asset and live in m_buffers_vertex / m_buffers_index (filled by Ir77PVAsset).
enum class Ir77PVBufferSlot : std::uint8_t { Camera, Lights, Instances, ShadowMatrices, Bones, Indirect };

// Color -- albedo / emissive: sRGB, mipmapped, sampled
// Data  -- normal / ORM / AO / masks: UNORM (no gamma), mipmapped, sampled
// Cube  -- env / IBL: 6 layers, sRGB by default (SetFormat for HDR), mipmapped, sampled
// Depth -- shadow map: D32, depth attachment + sampled, no mips, no upload
enum class Ir77PVTextureKind : std::uint8_t { Color, Data, Cube, Depth };

// Default textures bound to image slots that have no real texture yet
enum class Ir77PVPlaceholderKind : std::uint8_t { White, Normal };

typedef struct Ir77PVFrameSizes {
    VkDeviceSize camera{256};              // view, proj, view_proj, eye position
    VkDeviceSize lights{64 * 64};          // 64 lights x 64 bytes
    VkDeviceSize instances{8192 * 64};     // 8192 instances x mat4
    VkDeviceSize shadow_matrices{4 * 64};  // 4 cascades / lights x mat4
    VkDeviceSize bones{256 * 128 * 64};    // 256 skeletons x 128 joints x mat4
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

typedef struct Ir77PVDescriptorBinding {
    std::uint32_t binding{0};
    VkDescriptorType type{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
    std::shared_ptr<IIr77PVBuffer> buffer{};
    VkDescriptorImageInfo image{};
    bool is_image{false};
}* pIr77PVDescriptorBinding;

// =============================================================================================================================================
// vertex formats -- CPU-side input. Ir77PVVertex is also read as a raw float[8] stream by the mesh shader (set 3 binding 3).
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

typedef struct Ir77PVInputBuffer {
    std::vector<Ir77PVVertex> vertex_data{};
    std::vector<std::uint32_t> index_data{};
}* pIr77PVInputBuffer;

// =============================================================================================================================================
// shader-visible data -- layouts must match the GLSL blocks
// =============================================================================================================================================
// Set 0 binding 0. The camera block in vert.spv must match:
//   layout(set = 0, binding = 0) uniform Camera { mat4 view; mat4 proj; mat4 view_proj; vec4 eye; };
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

// Set 2 binding 0. std140-safe: vec4, then 4 floats packed into a vec4.
typedef struct Ir77PVMaterialConstants {
    std::array<float, 4> base_color{1.0f, 1.0f, 1.0f, 1.0f};
    std::array<float, 4> emissive{0.0f, 0.0f, 0.0f, 0.0f};
    float roughness{1.0f};
    float metallic{0.0f};
    float alpha_cutoff{0.5f};
    float normal_scale{1.0f};
}* pIr77PVMaterialConstants;

// Set 3 binding 0 (Mesh). std430: vec4, vec4, uvec4.
typedef struct Ir77PVMeshlet {
    std::array<float, 4> sphere{};  // xyz center, w radius -- object space
    std::array<float, 4> cone{};    // xyz axis, w cutoff
    std::uint32_t vertex_offset{0};
    std::uint32_t triangle_offset{0};
    std::uint32_t vertex_count{0};
    std::uint32_t triangle_count{0};
}* pIr77PVMeshlet;

static_assert(sizeof(Ir77PVMeshlet) == 48, "must match the std430 Meshlet struct in the task / mesh shaders");

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

// Uploaded mesh: device-local vertex + index buffers (single-mesh path, used by the testbed and any
// one-off load). Scene loading uses UploadScene instead -- one shared buffer per channel, not
// one buffer pair per mesh.
typedef struct Ir77PVMesh {
    std::shared_ptr<IIr77PVBuffer> vertex{};
    std::shared_ptr<IIr77PVBuffer> index{};
    std::uint32_t index_count{0};
    VkIndexType index_type{VK_INDEX_TYPE_UINT32};
}* pIr77PVMesh;

// Uploaded meshlet mesh: the four set 3 storage buffers plus their set (Mesh pipeline kind only)
typedef struct Ir77PVMeshletMesh {
    std::shared_ptr<IIr77PVBuffer> meshlets{};           // binding 0
    std::shared_ptr<IIr77PVBuffer> meshlet_vertices{};   // binding 1
    std::shared_ptr<IIr77PVBuffer> meshlet_triangles{};  // binding 2
    std::shared_ptr<IIr77PVBuffer> vertices{};           // binding 3
    std::shared_ptr<IIr77PVBuffer> draws{};
    std::shared_ptr<IIr77PVBuffer> indirect{};
    std::shared_ptr<IIr77PVDescriptorSet> set{};
    std::uint32_t meshlet_count{0};
}* pIr77PVMeshletMesh;

// Set 3 binding 4 (Mesh). std430: uvec4. draws[draw_base + gl_DrawID] says which object a draw is.
typedef struct Ir77PVMeshDraw {
    std::uint32_t instance{0};  // index into set 0 binding 2 (instances)
    std::uint32_t meshlet_offset{0};
    std::uint32_t meshlet_count{0};
    std::uint32_t _pad{0};
}* pIr77PVMeshDraw;

static_assert(sizeof(Ir77PVMeshDraw) == 16, "must match MeshDraw in mesh.task");

// Set 2 as a whole: constants buffer + descriptor set (textures 1..5 start as placeholders)
typedef struct Ir77PVMaterial {
    std::shared_ptr<IIr77PVBuffer> constants{};
    std::shared_ptr<IIr77PVDescriptorSet> set{};
}* pIr77PVMaterial;

// =============================================================================================================================================
// draw
// =============================================================================================================================================

// One draw in a pass list. Raster kinds use vertex / index; Mesh uses meshlets and meshlet_offset / meshlet_count.
typedef struct Ir77PVDrawItem {
    std::shared_ptr<IIr77PVPipeline> pipeline{};
    std::shared_ptr<IIr77PVLayout> layout{};
    std::shared_ptr<IIr77PVDescriptorSet> material{};  // set 2 -- Static / Skinned / Transparent / Mesh
    std::shared_ptr<IIr77PVDescriptorSet> bones{};     // set 3 Skinned, set 1 ShadowSkinned
    std::shared_ptr<IIr77PVDescriptorSet> meshlets{};  // set 3 Mesh -- meshlets, meshlet vertices, triangles, vertices
    std::shared_ptr<IIr77PVBuffer> vertex{};           // raster kinds only
    std::shared_ptr<IIr77PVBuffer> index{};            // raster kinds only
    VkIndexType index_type{VK_INDEX_TYPE_UINT32};
    std::uint32_t index_count{0};
    std::uint32_t first_index{0};
    std::int32_t vertex_offset{0};
    std::uint32_t instance_count{1};
    std::uint32_t first_instance{0};
    std::uint32_t meshlet_offset{0};  // Mesh -- first meshlet in the set 3 buffers, pushed to the task shader
    std::uint32_t meshlet_count{0};   // Mesh -- task groups = ceil(meshlet_count / task workgroup size)
    std::uint32_t draw_base{0};
    std::shared_ptr<IIr77PVBuffer> indirect{};  // VkDrawIndexedIndirectCommand, or VkDrawMeshTasksIndirectCommandEXT for Mesh
    VkDeviceSize indirect_offset{0};
    std::uint32_t indirect_count{0};
    std::array<float, 16> model{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    std::uint32_t light_index{0};  // shadow kinds only
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