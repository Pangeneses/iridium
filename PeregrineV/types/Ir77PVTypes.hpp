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
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../../Ir77RT/types/Ir77Types.hpp"

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// forward declarations -- interfaces held by shared_ptr in the structs below
// =============================================================================================================================================
struct IIr77PVBuffer;
struct IIr77PVPipeline;
struct IIr77PVTexture;

// =============================================================================================================================================
// limits / capacities
// =============================================================================================================================================
static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 3;

static constexpr std::size_t MAX_WINDOWS = 8;

// cameras one window draws at once -- each in its own area of the window (Ir77PVPassDesc::view / area), each with its own frame table
static constexpr std::uint32_t MAX_VIEWS = 4;

// bytes between views in the camera buffer and in the frame table buffer -- the largest minStorageBufferOffsetAlignment Vulkan allows, so
// every view's range is a legal descriptor offset on any device
static constexpr VkDeviceSize VIEW_STRIDE = 256;

// rows in the material table (one Static buffer per device)
static constexpr std::uint32_t MATERIAL_CAPACITY = 256;

static constexpr std::uint32_t SKELETON_CAPACITY = 64;

// meshoptimizer limits -- must match max_vertices / max_primitives in the mesh shader. Triangles must be divisible by 4.
static constexpr std::size_t MESHLET_MAX_VERTICES = 64;

static constexpr std::size_t MESHLET_MAX_TRIANGLES = 124;

// 0 = pure locality, higher = tighter normal cones for backface culling
static constexpr float MESHLET_CONE_WEIGHT = 0.25f;

/************************************************************************************************************************************************************************/
/********************************************************************** SYSTEM ID'S
 * ********************************************************************************/
/************************************************************************************************************************************************************************/
inline constexpr std::uint64_t ID_DEVICE_NONE = UINT64_MAX;

inline constexpr std::uint64_t ID_DEVICE_001 = 0x9B72E4C1F08A33D7;

inline constexpr std::uint64_t ID_DEVICE_002 = 0xA4F7C9D2035B7E11;

inline constexpr std::uint64_t ID_DEVICE_003 = 0x9B22E4A7F0C18D33;

inline constexpr std::uint64_t ID_DEVICE_004 = 0xC8D1A3F945667210;

inline constexpr std::uint64_t ID_DEVICE_005 = 0x7F33B8C2D9A4E5CC;

inline constexpr std::uint64_t ID_DEVICE_006 = 0xE1448F2297C3B508;

inline constexpr std::uint64_t ID_DEVICE_007 = 0x5D9A77C1E204F399;

inline constexpr std::uint64_t ID_DEVICE_008 = 0xB3C4E8F117A29D54;

// =============================================================================================================================================
// shader handles -- index into the device's shader registry (Ir77PVShader); stable for the registry's life. Shaders are found by name.
// =============================================================================================================================================
using Ir77PVShaderHandle = std::uint32_t;

inline constexpr Ir77PVShaderHandle IR77PV_NO_SHADER = UINT32_MAX;

// =============================================================================================================================================
// pipeline handles -- what a draw item carries. Issued by Ir77PVPipelineCache::FindPipeline per manifest pipeline name (or engine role),
// stable for the cache's life, resolved against the current manifest every time a command buffer records the item.
// =============================================================================================================================================
using Ir77PVPipelineHandle = std::uint32_t;

inline constexpr Ir77PVPipelineHandle IR77PV_NO_PIPELINE = UINT32_MAX;

// =============================================================================================================================================
// bindless -- one layout, one descriptor set per device. Must match Shader/include/bindless.glsl.
// =============================================================================================================================================

// set 0 binding numbers
inline constexpr std::uint32_t BINDLESS_BINDING_TEXTURES = 0;
inline constexpr std::uint32_t BINDLESS_BINDING_BUFFERS = 1;
inline constexpr std::uint32_t BINDLESS_BINDING_STORAGE_IMAGES = 2;
inline constexpr std::uint32_t BINDLESS_BINDING_ACCELERATION = 3;  // ray-query devices only -- the binding doesn't exist without them
inline constexpr std::uint32_t BINDLESS_BINDING_COUNT = 4;

// requested array sizes -- Ir77PVLayout clamps them to the device's update-after-bind limits
inline constexpr std::uint32_t BINDLESS_TEXTURES = 4096;
inline constexpr std::uint32_t BINDLESS_BUFFERS = 4096;
inline constexpr std::uint32_t BINDLESS_STORAGE_IMAGES = 256;
inline constexpr std::uint32_t BINDLESS_ACCELERATION_STRUCTURES = 16;  // the TLAS ring, with room to spare

// texture slot 0 always holds the white placeholder, so an unset texture slot samples something valid
inline constexpr std::uint32_t BINDLESS_PLACEHOLDER_TEXTURE = 0;

// "no slot" for optional buffer / texture slots
inline constexpr std::uint32_t BINDLESS_NO_SLOT = UINT32_MAX;

// What a bindless slot holds -- given at Register, kept by the table beside the slot, None once released. Debug builds check the slots
// a draw item brings (bones, meshlet buffers) against the kind its push-constant field needs before recording it.
enum class Ir77PVSlotKind : std::uint8_t {
    None,
    // binding 1 -- buffers
    FrameTable,
    Camera,
    Lights,
    Instances,
    ShadowMatrices,
    Bones,
    MaterialTable,
    Vertices,
    Meshlets,
    MeshletVertices,
    MeshletTriangles,
    MeshDraws,
    Storage,  // CreateStorageBuffer -- whatever the caller puts in it
    // binding 0 -- sampled images
    Texture,
    ShadowMap,
    Canvas,
    Overlay,
    // binding 2 -- storage images
    CanvasImage,
    // binding 3 -- acceleration structures
    TopLevel
};

// For the log -- same order as Ir77PVSlotKind
inline constexpr std::array<char const*, 20> SLOT_KIND_NAMES{"none",          "frame table",      "camera",         "lights",      "instances",
                                                             "shadow matrices", "bones",          "material table", "vertices",    "meshlets",
                                                             "meshlet vertices", "meshlet triangles", "mesh draws",  "storage",     "texture",
                                                             "shadow map",    "canvas",           "overlay",        "canvas image", "top level"};

// One slot -- which binding, which element
struct Ir77PVSlot {
    std::uint32_t binding{0};
    std::uint32_t index{BINDLESS_NO_SLOT};
};

// array sizes actually in use on this device
struct Ir77PVBindlessCapacity {
    std::uint32_t textures{0};
    std::uint32_t buffers{0};
    std::uint32_t storage_images{0};
    std::uint32_t acceleration_structures{0};  // 0 = no binding 3 (no ray queries)
};

// One push-constant block for every pipeline -- graphics, compute and ray. 128 bytes, the guaranteed minimum.
// Must match the Ir77PushConstants block in bindless.glsl byte for byte.
//   Mesh:   vertices = vertices slot, user[0] = meshlets, [1] = meshlet vertex indices, [2] = meshlet triangles, [3] = draw records
//   Shadow: user[0] = light index
//   CEF:    user[0] = overlay texture slot
struct Ir77PVPushConstants {
    std::array<float, 16> model{};        // column-major, like glm::mat4
    std::uint32_t frame{0};               // buffer slot: this frame's frame table
    std::uint32_t instances{0};           // buffer slot: instance table
    std::uint32_t materials{0};           // buffer slot: material table
    std::uint32_t material{0};            // row in the material table
    std::uint32_t vertices{0};            // buffer slot: pulled / mesh / compute vertex data
    std::uint32_t bones{0};               // buffer slot: bone palette
    std::uint32_t draw_base{0};           // first draw / meshlet for this call
    std::uint32_t mode{0};                // shading mode -> pc.mode
    std::array<std::uint32_t, 8> user{};  // per-kind data, see above
};

static_assert(sizeof(Ir77PVPushConstants) == 128, "Ir77PVPushConstants must be exactly 128 bytes");

// Frame table -- one row per window per frame in flight, in its own buffer slot (pc.frame): the slots of that frame's buffers and textures.
// std430, 32 bytes. Must match Ir77FrameTable in bindless.glsl.
struct Ir77PVFrameTable {
    std::uint32_t camera{BINDLESS_NO_SLOT};                  // buffer slot: Ir77PVCamera
    std::uint32_t lights{BINDLESS_NO_SLOT};                  // buffer slot: Ir77PVLightsHeader + Ir77PVLight[]
    std::uint32_t instances{BINDLESS_NO_SLOT};               // buffer slot: mat4[] -- also pc.instances
    std::uint32_t shadow_matrices{BINDLESS_NO_SLOT};         // buffer slot: mat4[]
    std::uint32_t bones{BINDLESS_NO_SLOT};                   // buffer slot: bone bases + palettes -- pc.bones when a draw has none
    std::uint32_t env{BINDLESS_PLACEHOLDER_TEXTURE};         // texture slot: environment / IBL
    std::uint32_t shadow_map{BINDLESS_PLACEHOLDER_TEXTURE};  // texture slot: shadow map
    std::uint32_t tlas{BINDLESS_NO_SLOT};                    // acceleration-structure slot (binding 3): the TLAS built this frame; NO_SLOT = none
    std::uint32_t ray_objects{BINDLESS_NO_SLOT};             // buffer slot: Ir77PVRayObject[] -- what a ray hit is (custom index = row); NO_SLOT = none
};

static_assert(sizeof(Ir77PVFrameTable) == 36, "Ir77PVFrameTable must match Ir77FrameTable in bindless.glsl");

static_assert(sizeof(Ir77PVFrameTable) <= VIEW_STRIDE, "a view's frame table must fit its stride");

// Per window per frame: one frame table per view (its slot is pc.frame for that view's passes), the camera slot each one lists, and a CPU
// copy of everything else they hold -- the same for every view (Ir77PVCmdBuffer builds push constants from it). values.camera is view 0's.
struct Ir77PVFrameSlots {
    std::array<std::uint32_t, MAX_VIEWS> tables{BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT};
    std::array<std::uint32_t, MAX_VIEWS> cameras{BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT};
    Ir77PVFrameTable values{};
};

static_assert(MAX_VIEWS == 4, "Ir77PVFrameSlots spells out one entry per view");

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
// pass list -- what Ir77PVCmdBuffer records each frame, in order. Draw lists (Ir77PVPass) are filled by SetDraws; a pass says which lists it
// records and into what. Default: Shadow pass -> Main pass (Opaque, Transparent, CEF overlay).
// =============================================================================================================================================

// Compute -- the dispatch list (SetDispatches), outside any rendering
// Raster  -- vkCmdBeginRendering on the target, then the pass's draw lists in order
enum class Ir77PVPassType : std::uint8_t { Compute, Raster };

// What a Raster pass renders into
// Swapchain -- the acquired swapchain image + the swapchain's depth image. Any number of passes may target it, each on its own area: the
//             first clears the whole image, the rest load what is there, the last ends ready to present.
// Shadow    -- the shadow target (SetShadowTarget), depth only; ends ready to be sampled. Skipped while it has no draws or no target.
enum class Ir77PVPassTarget : std::uint8_t { Swapchain, Shadow };

struct Ir77PVPassDesc {
    Ir77PVPassType type{Ir77PVPassType::Raster};
    Ir77PVPassTarget target{Ir77PVPassTarget::Swapchain};
    Ir77PVRenderPassKind formats{Ir77PVRenderPassKind::Main};  // the attachment formats the pass's pipelines were built for
    std::vector<Ir77PVPass> lists{};                           // draw lists, recorded in this order
    bool overlay{false};                                       // CEF composite after the lists
    std::uint32_t view{0};                                     // whose draw lists it records and whose frame table (camera) they see
    glm::vec4 area{0.0f, 0.0f, 1.0f, 1.0f};                    // where on the target -- x, y, width, height as fractions of its extent
};

// =============================================================================================================================================
// pipelines -- a pipeline is described by Ir77PVPipelineDesc and built on first request by the device's Ir77PVPipelineCache.
// Every pipeline uses the one bindless layout. Cull mode, front face and depth test / write / compare are dynamic: the description carries
// the policy Ir77PVCmdBuffer applies per draw, the pipeline itself doesn't bake them.
// =============================================================================================================================================

// Presets -- Ir77PVPipelineCache::DescribeKind turns each into a description
// Static / Skinned / *Transparent -- vertex input, main pass. Transparent far / near sides: one pipeline, Ir77PVDrawItem::back_faces
// Shadow / ShadowSkinned          -- vertex input, depth-only shadow pass
// CEF                             -- fullscreen quad, overlay texture from pc.user[0]
// Mesh                            -- task / mesh / fragment, no vertex input (VK_EXT_mesh_shader only)
enum class Ir77PVPipelineKind : std::uint8_t { Static, Skinned, Shadow, ShadowSkinned, Transparent, CEF, Mesh, SkinnedTransparent };

// Opaque -- no blending. Alpha -- straight alpha over the target. Additive -- src * alpha added to the target (glow, sparks; order-free).
enum class Ir77PVBlend : std::uint8_t { Opaque, Alpha, Additive };

struct Ir77PVPipelineDesc {
    // stages -- registry handles (Ir77PVShader::FindShader / LoadShader). Raster: vertex [+ fragment]. Mesh: [task +] mesh [+ fragment].
    // Vertex input comes from the vertex shader's reflected input locations -- see VERTEX_LOCATION_*.
    Ir77PVShaderHandle vertex{IR77PV_NO_SHADER};
    Ir77PVShaderHandle fragment{IR77PV_NO_SHADER};
    Ir77PVShaderHandle task{IR77PV_NO_SHADER};
    Ir77PVShaderHandle mesh{IR77PV_NO_SHADER};
    Ir77PVShaderHandle compute{IR77PV_NO_SHADER};  // a compute pipeline -- no other stage, nothing below applies

    // baked into the pipeline
    Ir77PVBlend blend{Ir77PVBlend::Opaque};
    Ir77PVRenderPassKind pass{Ir77PVRenderPassKind::Main};  // attachment formats
    bool depth_bias{false};                                 // shadow acne
    bool flip_back_faces{true};                             // frag.frag constant_id 0

    // dynamic -- applied per draw by Ir77PVCmdBuffer
    VkCullModeFlags cull{VK_CULL_MODE_BACK_BIT};  // an item's back_faces / double_sided adjusts this
    bool depth_test{true};
    bool depth_write{true};

    // task / mesh shading -- no vertex input stage
    bool IsMesh() const { return mesh != IR77PV_NO_SHADER; }

    // compute -- dispatched (Ir77PVDispatch), never drawn
    bool IsCompute() const { return compute != IR77PV_NO_SHADER; }

    // Cache key -- every field compares, so two descriptions share a pipeline only when they are identical
    auto operator<=>(Ir77PVPipelineDesc const&) const = default;
};

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

// The contract every shader meets -- checked from its SPIR-V when it is registered (Ir77PVShader), so a shader that breaks it is rejected at
// load with a message instead of failing at draw time:
//   entry point "main"; descriptors only in set 0 -- binding 0 sampled textures, 1 storage buffers, 2 storage images (bindless.glsl);
//   push constants at most 128 bytes (Ir77PVPushConstants); vertex shaders read only the vertex locations below; task / mesh shaders only on a
//   device with mesh shading.
inline constexpr std::uint32_t PUSH_CONSTANT_LIMIT = 128;

// Vertex input locations, binding 0. A vertex shader reading 3 or 4 is fed the skinned layout (Ir77PVVertexSkinned), otherwise Ir77PVVertex.
// 5 (tangent) is in both.
inline constexpr std::uint32_t VERTEX_LOCATION_POSITION = 0;
inline constexpr std::uint32_t VERTEX_LOCATION_NORMAL = 1;
inline constexpr std::uint32_t VERTEX_LOCATION_UV = 2;
inline constexpr std::uint32_t VERTEX_LOCATION_JOINTS = 3;
inline constexpr std::uint32_t VERTEX_LOCATION_WEIGHTS = 4;
inline constexpr std::uint32_t VERTEX_LOCATION_TANGENT = 5;

// What a shader's SPIR-V says about itself
struct Ir77PVShaderReflection {
    std::vector<std::uint32_t> inputs{};   // Input locations, sorted (built-ins excluded)
    std::vector<std::uint32_t> outputs{};  // Output locations, sorted (built-ins excluded)
    std::uint32_t push_constant_size{0};   // bytes; 0 = no push constants
};

// One registered shader -- its name, stage (from the SPIR-V entry point), module (in stage_create_info), reflection and SPIR-V
struct Ir77PVShaderInfo {
    std::string name{};
    Ir77PVShaderStage stage{Ir77PVShaderStage::Vertex};
    VkPipelineShaderStageCreateInfo stage_create_info{};
    Ir77PVShaderReflection reflection{};
    std::vector<std::uint32_t> spirv{};
};

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
// Storage -- compute-written canvas: RGBA8, storage image + sampled, always VK_IMAGE_LAYOUT_GENERAL
enum class Ir77PVTextureKind : std::uint8_t { Color, Data, Cube, Depth, Storage };

// Default textures for texture slots that have no real texture yet. White is always texture slot 0.
enum class Ir77PVPlaceholderKind : std::uint8_t { White, Normal };

struct Ir77PVFrameSizes {
    VkDeviceSize camera{VIEW_STRIDE * MAX_VIEWS};   // one Ir77PVCamera per view, VIEW_STRIDE apart
    VkDeviceSize lights{16 + 64 * 64};              // 16-byte header + 64 lights x 64 bytes
    VkDeviceSize instances{8192 * 64};              // 8192 instances x mat4
    VkDeviceSize shadow_matrices{4 * 64};           // 4 cascades / lights x mat4
    VkDeviceSize bones{8192 * 4 + 256 * 128 * 64};  // bone_base per instance slot + 256 skeletons x 128 joints x mat4
    VkDeviceSize indirect{8192 * sizeof(VkDrawIndexedIndirectCommand)};
};

// One destination buffer + the data going into it, for a batched multi-buffer upload
// (one staging buffer, one command buffer, one submit covering several entries at once).
struct Ir77PVUploadEntry {
    VkBuffer dst{VK_NULL_HANDLE};
    void const* data{nullptr};
    VkDeviceSize size{0};
    VkDeviceSize dst_offset{0};  // where in dst this entry lands -- 0 = start of the buffer
};

struct Ir77PVSamplerDesc {
    VkFilter filter{VK_FILTER_LINEAR};
    VkSamplerAddressMode address{VK_SAMPLER_ADDRESS_MODE_REPEAT};
    float anisotropy{0.0f};  // 0 = off; >0 requires samplerAnisotropy enabled on the device
    bool compare{false};     // depth compare (sampler2DShadow / PCF); Depth kind only
};

inline constexpr std::uint32_t IR77PV_NO_TEXTURE = UINT32_MAX;

// Texture ids (Ir77PVAsset::CreateTexture) for a material's maps. IR77PV_NO_TEXTURE = placeholder (flat normal for the normal map).
struct Ir77PVMaterialTextures {
    std::uint32_t albedo{IR77PV_NO_TEXTURE};
    std::uint32_t normal{IR77PV_NO_TEXTURE};
    std::uint32_t orm{IR77PV_NO_TEXTURE};
    std::uint32_t emissive{IR77PV_NO_TEXTURE};
    std::uint32_t ao{IR77PV_NO_TEXTURE};
};

// =============================================================================================================================================
// vertex formats -- CPU-side input. Ir77PVVertex is also read as a raw float[12] stream by the mesh shader and skin.comp (pc.vertices).
// tangent: xyz runs along +U on the surface, w = handedness (+-1). bitangent = cross(normal, tangent.xyz) * w.
// =============================================================================================================================================
struct Ir77PVVertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
};

static_assert(sizeof(Ir77PVVertex) == sizeof(float) * 12, "mesh shader / skin.comp read vertices as 12 tightly packed floats");

// skin.comp reads it as 20 words: position 3, normal 3, uv 2, tangent 4, joints 4 (uint), weights 4
struct Ir77PVVertexSkinned {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
    glm::uvec4 joints{};
    glm::vec4 weights{};
};

static_assert(sizeof(Ir77PVVertexSkinned) == sizeof(std::uint32_t) * 20, "skin.comp reads skinned vertices as 20 tightly packed words");

// =============================================================================================================================================
// shader-visible data -- std430, layouts must match the GLSL structs
// =============================================================================================================================================

// frame table .camera:  struct Camera { mat4 view; mat4 proj; mat4 view_proj; vec4 eye; };
struct Ir77PVCamera {
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
    glm::mat4 view_proj{1.0f};
    glm::vec4 eye{0.0f};

    // x seconds since start, y seconds since last frame, z frame number -- the caller's clock
    glm::vec4 time{0.0f};
};

static_assert(sizeof(Ir77PVCamera) <= VIEW_STRIDE, "a view's camera must fit its stride");

struct Ir77PVLight {
    glm::vec3 position{};
    std::int32_t shadow{-1};  // shadow atlas tile = ShadowMatrices index; -1 = casts no shadow
    glm::vec3 direction{};
    float _pad1{};
    glm::vec3 color{};
    float intensity{};
    float range{};
    float innerConeAngle{};
    float outerConeAngle{};
    int type{};
};

static_assert(sizeof(Ir77PVLight) == 64, "lights buffer is read as 64-byte std430 structs");

// the shadow map is an atlas of SHADOW_ATLAS_SIDE x SHADOW_ATLAS_SIDE tiles, one per shadow-casting light, tile i at (i % side, i / side).
// Must match IR77_SHADOW_ATLAS_SIDE in bindless.glsl; ShadowMatrices holds MAX_SHADOW_LIGHTS matrices (Ir77PVFrameSizes::shadow_matrices).
inline constexpr std::uint32_t SHADOW_ATLAS_SIDE = 2;
inline constexpr std::uint32_t MAX_SHADOW_LIGHTS = SHADOW_ATLAS_SIDE * SHADOW_ATLAS_SIDE;

// frame table .lights header -- the light count, padded to 16 bytes; the Ir77PVLight array follows it
struct Ir77PVLightsHeader {
    std::uint32_t count{0};
    std::uint32_t _pad[3]{};
};

static_assert(sizeof(Ir77PVLightsHeader) == 16, "lights header must stay 16 bytes so the array stays aligned");

// Material constants -- the head of each material-table row. vec4, vec4, then 4 floats; must stay a 16-byte multiple.
struct Ir77PVMaterialConstants {
    std::array<float, 4> base_color{1.0f, 1.0f, 1.0f, 1.0f};
    std::array<float, 4> emissive{0.0f, 0.0f, 0.0f, 0.0f};
    float roughness{1.0f};
    float metallic{0.0f};
    float alpha_cutoff{0.5f};
    float normal_scale{1.0f};
};

static_assert(sizeof(Ir77PVMaterialConstants) % 16 == 0, "Ir77PVMaterialConstants must be a 16-byte multiple for the std430 material table");

// One row of the material table (pc.materials, row pc.material). Must match Ir77MaterialRow in bindless.glsl.
struct Ir77PVMaterialRow {
    Ir77PVMaterialConstants constants{};
    std::array<std::uint32_t, 4> textures0{};  // texture slots: albedo, normal, ORM, emissive
    std::array<std::uint32_t, 4> textures1{};  // texture slots: AO, -, -, -
};

static_assert(sizeof(Ir77PVMaterialRow) == 80, "Ir77PVMaterialRow must match Ir77MaterialRow in bindless.glsl");

// Meshlet record (mesh path, pc.user[0]). std430: vec4, vec4, uvec4, uvec4.
struct Ir77PVMeshlet {
    std::array<float, 4> sphere{};  // xyz center, w radius -- object space
    std::array<float, 4> cone{};    // xyz axis, w cutoff
    std::uint32_t vertex_offset{0};
    std::uint32_t triangle_offset{0};
    std::uint32_t vertex_count{0};
    std::uint32_t triangle_count{0};
    std::uint32_t material{0};  // row in the material table -- every triangle in the meshlet uses it
    std::array<std::uint32_t, 3> _pad{};
};

static_assert(sizeof(Ir77PVMeshlet) == 64, "must match the std430 Meshlet struct in the task / mesh shaders");

// Draw record (mesh path, pc.user[3]). std430: uvec4. draws[draw_base + gl_DrawID] says which object a draw is.
struct Ir77PVMeshDraw {
    std::uint32_t instance{0};  // index into the instance table
    std::uint32_t meshlet_offset{0};
    std::uint32_t meshlet_count{0};
    std::uint32_t _pad{0};
};

static_assert(sizeof(Ir77PVMeshDraw) == 16, "must match MeshDraw in mesh.task");

// Indirect command record -- same layout as VkDrawIndexedIndirectCommand
struct Ir77MeshRange {
    std::uint32_t index_count{0};
    std::uint32_t instance_count{0};
    std::uint32_t index_offset{0};
    std::int32_t vertex_offset{0};
    std::uint32_t first_instance{0};
};

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
struct Ir77PVSceneUpload {
    std::vector<Ir77PVVertex> vertices{};
    std::vector<std::uint32_t> indices{};
    std::vector<Ir77PVVertexSkinned> vertices_skinned{};
    std::vector<std::uint32_t> indices_skinned{};
    std::vector<VkDrawIndexedIndirectCommand> indirect_commands{};
    std::vector<std::uint32_t> triangle_materials{};  // mesh path: material row per triangle of `indices` (indices.size() / 3)
};

struct Ir77PVSingleMesh {
    std::vector<Ir77PVVertex> vertex_data{};
    std::vector<std::uint32_t> index_data{};
};

// Uploaded mesh: device-local vertex + index buffers (single-mesh path, used by the testbed and any
// one-off load). Scene loading uses UploadScene instead -- one shared buffer per channel, not
// one buffer pair per mesh.
struct Ir77PVSingleMeshBuffer {
    std::shared_ptr<IIr77PVBuffer> vertex{};
    std::shared_ptr<IIr77PVBuffer> index{};
    std::uint32_t index_count{0};
    VkIndexType index_type{VK_INDEX_TYPE_UINT32};
};

// A material is a row in the material table
struct Ir77PVMaterialData {
    std::uint32_t row{0};
};

// Meshlet buffers and their bindless slots. Slot order matches Ir77PVDrawItem::meshlet_slots:
//   [0] meshlets   [1] meshlet vertex indices   [2] meshlet triangles   [3] vertices   [4] draw records
inline constexpr std::size_t MESHLET_SLOT_COUNT = 5;

struct Ir77PVMeshletMesh {
    std::shared_ptr<IIr77PVBuffer> meshlets{};
    std::shared_ptr<IIr77PVBuffer> meshlet_vertices{};
    std::shared_ptr<IIr77PVBuffer> meshlet_triangles{};
    std::shared_ptr<IIr77PVBuffer> vertices{};
    std::shared_ptr<IIr77PVBuffer> draws{};
    std::shared_ptr<IIr77PVBuffer> indirect{};
    std::array<std::uint32_t, MESHLET_SLOT_COUNT> slots{BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT};
    std::uint32_t meshlet_count{0};
};

// vertex path scene -- one buffer per channel for every stacked object
struct Ir77PVSceneSlot {
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
    std::uint32_t vertex_slot{BINDLESS_NO_SLOT};          // GetSceneVertexSlot -- the vertex buffer as a storage buffer
    std::uint32_t vertex_skinned_slot{BINDLESS_NO_SLOT};  // the same for the skinned channel
    std::uint32_t vertex_count{0};                        // vertices in `vertex` -- the BLAS builds' upper bound
};

// What Acceleration reads out of a vertex-path scene (Ir77PVGeometry::GetSceneGeometry) -- the buffers it builds from and the commands
// that cut them into objects
struct Ir77PVSceneGeometry {
    std::shared_ptr<IIr77PVBuffer> vertex{};
    std::shared_ptr<IIr77PVBuffer> index{};
    std::uint32_t vertex_count{0};
    std::vector<VkDrawIndexedIndirectCommand> commands{};
    std::vector<std::uint32_t> instance_counts{};  // per command, whatever its visibility
};

// mesh path scene -- meshlet buffers plus the per-record command state
struct Ir77PVMeshletSlot {
    Ir77PVMeshletMesh mesh{};
    std::vector<VkDrawMeshTasksIndirectCommandEXT> commands{};  // live, one per record -- groupCountX 0 = hidden
    std::vector<std::uint32_t> groups{};                        // what a visible record dispatches
    std::vector<std::uint32_t> first_record{};                  // per scene command, plus one end entry
    std::uint32_t dirty{0};
    bool alive{false};
    // ray queries -- what Acceleration builds the static stream's BLASes from (GetMeshletGeometry). ray_index is the scene's index buffer,
    // kept only on ray-query devices; the vertices are mesh.vertices.
    std::shared_ptr<IIr77PVBuffer> ray_index{};
    std::uint32_t vertex_count{0};
    std::vector<VkDrawIndexedIndirectCommand> scene_commands{};  // the upload's indirect commands -- one BLAS each
    std::vector<std::uint32_t> instance_counts{};                // per scene command
};

// What an owner let go of -- held by Transfer's graveyard until every frame that could still read it has finished, then released in one
// step: the slots back to the bindless table, the buffers and textures dropped, released() run. The only place that waits out the frames
// in flight. Empty entries (null pointers, BINDLESS_NO_SLOT) are skipped.
struct Ir77PVGrave {
    std::vector<std::shared_ptr<IIr77PVBuffer>> buffers{};
    std::vector<std::shared_ptr<IIr77PVTexture>> textures{};
    std::vector<Ir77PVSlot> slots{};
    std::function<void()> released{};  // anything else that had to wait -- material rows going back to their free list
    std::uint32_t frames_left{0};      // set by Bury
};

// =============================================================================================================================================
// draw
// =============================================================================================================================================

// One draw in a pass list. Everything a shader reads beyond vertex input comes through push constants built from these fields
// (Ir77PVCmdBuffer). Raster kinds use vertex / index; Mesh uses meshlet_slots and meshlet_offset / meshlet_count.
struct Ir77PVDrawItem {
    Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};  // FindPipeline(name / kind) -- NO_PIPELINE items are skipped

    std::uint32_t material{0};              // row in the material table -> pc.material
    std::uint32_t bones{BINDLESS_NO_SLOT};  // buffer slot of this draw's bone palette; NO_SLOT = the frame's bones buffer -> pc.bones
    std::uint32_t mode{0};                  // shading mode -> pc.mode
    bool double_sided{false};               // opaque pipelines draw with no culling (glTF doubleSided)
    bool mirrored{false};                   // negative-determinant model -- front face flips to clockwise
    bool back_faces{false};                 // draw the far side only (front faces culled) -- the first of a transparent object's two draws

    std::array<std::uint32_t, 4> user{};  // -> pc.user[4..7] -- the pipeline's manifest "user" names, in order

    // raster kinds. vertex may be empty when the pipeline's vertex shader reads no inputs (procedural: gl_VertexIndex / storage buffers).
    std::shared_ptr<IIr77PVBuffer> vertex{};
    std::shared_ptr<IIr77PVBuffer> index{};
    VkIndexType index_type{VK_INDEX_TYPE_UINT32};
    std::uint32_t index_count{0};  // indices -- or vertices, for a draw without an index buffer
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
    std::uint32_t light_index{0};                                                                                                 // shadow pass -> pc.user[0]
};

// One compute dispatch -- the Compute pass records the list in order, each followed by a barrier, so a dispatch sees the ones before it and
// every draw sees them all. pipeline: FindPipeline on a manifest entry with "compute". push: what the shader reads (slots in user[],
// vertices ...); frame / instances / bones / materials are filled in at record time.
struct Ir77PVDispatch {
    Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};
    Ir77PVPushConstants push{};
    std::array<std::uint32_t, 3> groups{1, 1, 1};

    // indirect -- groups from a VkDispatchIndirectCommand another dispatch wrote; groups ignored
    std::shared_ptr<IIr77PVBuffer> indirect{};
    VkDeviceSize indirect_offset{0};
};

// =============================================================================================================================================
// world -- what the renderer is handed: assets once (LoadAsset), then one frame at a time (SubmitFrame). Ir77PVWorld turns them into
// uploads, material tables, acceleration structures, frame data and draw lists.
// =============================================================================================================================================

inline constexpr std::uint32_t IR77PV_NO_ASSET = UINT32_MAX;

inline constexpr std::uint32_t IR77PV_NO_SURFACE = UINT32_MAX;  // CreateSurfaceAcceleration on a device without ray queries

// models[] slots and joint matrices every loaded asset shares -- must match Ir77PVFrameSizes::instances / bones and bone_base[] in skinned.vert
inline constexpr std::uint32_t WORLD_INSTANCE_CAPACITY = 8192;
inline constexpr std::uint32_t WORLD_BONE_CAPACITY = 256 * 128;

// One decoded texture -- RGBA8, tightly packed. srgb = color data (albedo, emissive); data maps (normal, ORM, AO) are linear.
struct Ir77PVAssetTexture {
    std::uint32_t width{0};
    std::uint32_t height{0};
    bool srgb{false};
    std::vector<std::uint8_t> rgba{};
};

// One material. textures name the bundle's textures by index (IR77PV_NO_TEXTURE = placeholder).
//   blend: BLEND -- drawn in the Transparent pass, one object at a time, back to front
//   mask:  MASK  -- alpha-tested by the rays too (the raster pipelines test alpha_cutoff themselves)
struct Ir77PVAssetMaterial {
    Ir77PVMaterialConstants constants{};
    Ir77PVMaterialTextures textures{};
    bool blend{false};
    bool mask{false};
};

// One primitive: its slice of the bundle's indices / vertices, the instances it draws through, its material. range.first_instance is
// the asset's own slot (0 .. instance_count - 1) -- LoadAsset moves it to wherever the asset lands in models[].
struct Ir77PVAssetPrimitive {
    Ir77MeshRange range{};
    std::uint32_t material{UINT32_MAX};  // index into materials; out of range -> the grey default
};

// One asset (one glTF file) -- geometry stacked per channel, its textures and materials, how many models[] slots and joint matrices it uses.
struct Ir77PVAssetBundle {
    std::string name{};  // for the log

    std::vector<Ir77PVVertex> vertices{};
    std::vector<std::uint32_t> indices{};
    std::vector<Ir77PVAssetPrimitive> primitives{};

    std::vector<Ir77PVVertexSkinned> vertices_skinned{};
    std::vector<std::uint32_t> indices_skinned{};
    std::vector<Ir77PVAssetPrimitive> primitives_skinned{};

    std::vector<Ir77PVAssetTexture> textures{};
    std::vector<Ir77PVAssetMaterial> materials{};

    std::uint32_t instance_count{0};          // models[] slots -- SubmitFrame's transforms for it, in this order
    std::vector<std::uint32_t> bone_bases{};  // per instance: where its joint palette starts in the asset's bones (0 when unskinned)
    std::uint32_t bone_count{0};              // joint matrices -- SubmitFrame's bones for it

    bool use_mesh{false};                                 // static opaque geometry on the mesh path when the device has it
    Ir77PVPipelineHandle pipeline{IR77PV_NO_PIPELINE};  // what the static opaque geometry draws with; NO_PIPELINE -> the Static role
};

// What LoadAsset made of a bundle
struct Ir77PVAssetReport {
    std::string device{};  // the GPU it went to
    bool mesh_path{false};
    bool ray_query{false};
    std::uint32_t instance_base{0};  // its first models[] slot
    std::uint32_t opaque_runs{0};    // material runs in the static / skinned opaque streams
    std::uint32_t skinned_runs{0};
    std::uint32_t transparent_objects{0};
};

// One asset in a frame. An asset left out of a frame is neither drawn nor in the rays.
struct Ir77PVFrameAsset {
    std::uint32_t asset{IR77PV_NO_ASSET};
    std::vector<glm::mat4> transforms{};  // instance_count world matrices
    std::vector<glm::mat4> bones{};       // bone_count joint matrices
};

// One window in a frame -- its cameras, and draws the assets don't make (a field), appended after theirs in every view.
// cameras: one per view, 1 .. MAX_VIEWS. One fills the window; 2 to 4 tile it 2 x 2, left to right then top to bottom -- every cell keeps the
// window's aspect, so each camera is built as if it had the whole window.
struct Ir77PVFrameWindow {
    std::vector<Ir77PVCamera> cameras{};
    std::vector<Ir77PVDrawItem> opaque{};
    std::vector<Ir77PVDrawItem> shadow{};
    std::vector<Ir77PVDrawItem> transparent{};
};

// Everything one frame needs. lights in engine units, each with its shadow tile; shadow_matrices[tile]. dispatches run after the assets'
// skinning, once a frame.
struct Ir77PVFrame {
    std::vector<Ir77PVFrameWindow> windows{};
    std::vector<Ir77PVLight> lights{};
    std::vector<glm::mat4> shadow_matrices{};
    std::vector<Ir77PVFrameAsset> assets{};
    std::vector<Ir77PVDispatch> dispatches{};
};

// What SubmitFrame drew -- the last window's lists
struct Ir77PVFrameReport {
    std::uint32_t opaque_items{0};
    std::uint32_t shadow_items{0};
    std::uint32_t transparent_items{0};
};

// =============================================================================================================================================
// ray tracing
// =============================================================================================================================================

// ---- acceleration structures (ray queries) -- Ir77PVAcceleration ----

// One object's BLAS: the triangles of one scene command (or one surface), read from its buffers by GPU address.
//   static:  built once at load, fastest to trace
//   dynamic: refit every frame (the posed stream -- same triangles, moved vertices)
//   rebuild: built from scratch every frame (a field surface -- its shape is whatever the program wrote this frame)
// material = the material row its alpha test reads (MASK materials) -- BINDLESS_NO_SLOT = opaque, never tested.
struct Ir77PVBottomLevel {
    VkAccelerationStructureKHR structure{VK_NULL_HANDLE};
    std::shared_ptr<IIr77PVBuffer> storage{};
    VkDeviceAddress address{0};  // what TLAS instances point at
    std::uint32_t scene_id{0};
    std::uint32_t command{0};
    bool dynamic{false};
    VkDeviceAddress vertex_address{0};
    VkDeviceAddress index_address{0};
    std::uint32_t max_vertex{0};
    std::uint32_t first_index{0};
    std::int32_t vertex_offset{0};
    std::uint32_t triangle_count{0};
    VkDeviceSize build_scratch{0};
    VkDeviceSize update_scratch{0};
    VkDeviceSize vertex_stride{sizeof(Ir77PVVertex)};  // bytes; position = the first 3 floats
    std::uint32_t material{BINDLESS_NO_SLOT};         // alpha-test material row; NO_SLOT = opaque
    bool rebuild{false};
    bool visible{true};  // in the TLAS
    bool alive{true};    // false once released -- its row stays, empty
};

// One TLAS entry -- a BLAS through one models[] slot. The hit's custom index is the BLAS (its Ir77PVRayObject row).
struct Ir77PVTopInstance {
    std::uint32_t bottom{0};         // index into Acceleration's BLAS list
    std::uint32_t slot{UINT32_MAX};  // models[] slot -- its transform; UINT32_MAX = identity (world-space geometry: a field surface)
};

// One BLAS as the shaders see it (IR77_FRAME.ray_objects, row = the hit's custom index) -- enough to rebuild a hit triangle's UV for the alpha
// test. Must match Ir77RayObject in include/raytrace.glsl. Addresses split low / high (no 64-bit ints in the shaders).
struct Ir77PVRayObject {
    std::array<std::uint32_t, 2> vertex_address{};
    std::array<std::uint32_t, 2> index_address{};
    std::uint32_t first_index{0};
    std::int32_t vertex_offset{0};
    std::uint32_t vertex_floats{0};            // vertex stride in floats -- UV at +6 (Ir77PVVertex)
    std::uint32_t material{BINDLESS_NO_SLOT};  // material row; NO_SLOT = opaque
};

static_assert(sizeof(Ir77PVRayObject) == 32, "Ir77PVRayObject must match Ir77RayObject in raytrace.glsl");

// One TLAS -- one per frame in flight
struct Ir77PVTopLevel {
    VkAccelerationStructureKHR structure{VK_NULL_HANDLE};
    std::shared_ptr<IIr77PVBuffer> storage{};
};

// SBT region a group's handle is written into
enum class Ir77PVRayRegion : std::uint8_t { RayGen, Miss, Hit, Callable };

// One shader group. Indices refer to Ir77PVPipelineRay::SetStages.
// General (raygen / miss / callable) sets `general`; triangle hit groups set closest_hit / any_hit;
// procedural hit groups also set intersection.
struct Ir77PVRayGroup {
    Ir77PVRayRegion region{Ir77PVRayRegion::RayGen};
    VkRayTracingShaderGroupTypeKHR type{VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR};
    std::uint32_t general{VK_SHADER_UNUSED_KHR};
    std::uint32_t closest_hit{VK_SHADER_UNUSED_KHR};
    std::uint32_t any_hit{VK_SHADER_UNUSED_KHR};
    std::uint32_t intersection{VK_SHADER_UNUSED_KHR};
};

}  // namespace NSIr77PeregrineV