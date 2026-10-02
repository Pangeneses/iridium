#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>
#include <memory>

namespace NSIr77PeregrineV {
static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 3;

// Which render pass a draw list is recorded into.
// Shadow       -> depth-only shadow pass (Shadow / ShadowSkinned pipelines)
// Opaque       -> main pass, first (Static / Skinned pipelines)
// Transparent -> main pass, after opaque; caller sorts back to front (Transparent pipeline)
enum class Ir77PVPass : std::uint8_t { Shadow, Opaque, Transparent };

// Attachment 0 is always color when a pass has color; depth is the last attachment.
// Main         -- swapchain color + depth, ends PRESENT_SRC (scene + CEF, today)
// Shadow    -- depth only, ends DEPTH_STENCIL_READ_ONLY (sampled later) (shadow maps)
// Offscreen  -- color + depth, color ends SHADER_READ_ONLY (HDR scene target, later)
// Post           -- swapchain color only, ends PRESENT_SRC (tonemap / fullscreen passes + CEF, later)
enum class Ir77PVRenderPassKind : std::uint8_t { Main, Shadow, Offscreen, Post };

enum class Ir77PVLayoutKind : std::uint8_t { Static, Skinned, Shadow, ShadowSkinned, CEF };

// Pipeline kind -> layout kind
// Static                  -> Ir77PVLayoutKind::Static
// Skinned              -> Ir77PVLayoutKind::Skinned
// Shadow              -> Ir77PVLayoutKind::Shadow         (depth-only render pass)
// ShadowSkinned -> Ir77PVLayoutKind::ShadowSkinned  (depth-only render pass)
// Transparent        -> Ir77PVLayoutKind::Static
// CEF                    -> Ir77PVLayoutKind::CEF
enum class Ir77PVPipelineKind : std::uint8_t { Static, Skinned, Shadow, ShadowSkinned, Transparent, CEF };

// Dynamic: host-mapped, one copy per frame in flight, written with Update() every frame
// Static:  device-local, single copy, written once with Upload() through a staging buffer
enum class Ir77PVBufferMode : std::uint8_t { Dynamic, Static };

// Color -- albedo / emissive: sRGB, mipmapped, sampled
// Data  -- normal / ORM / AO / masks: UNORM (no gamma), mipmapped, sampled
// Cube  -- env / IBL: 6 layers, sRGB by default (SetFormat for HDR), mipmapped, sampled
// Depth -- shadow map: D32, depth attachment + sampled, no mips, no upload
enum class Ir77PVTextureKind : std::uint8_t { Color, Data, Cube, Depth };

// Culling
// etc
// graphics specific 
enum class Ir77PVComputeKind : std::uint8_t {
    Culling,
    SkinningUpdate,
    MorphUpdate,
    ClothSim,
    ParticleSim,
    TerrainLOD,
    GPUOcclusion,
    IndirectPrep
};

typedef struct Ir77PVVertex {
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec2 uv{};
}* pIr77PVVertex;

typedef struct Ir77PVCamera {
    alignas(16) glm::mat4 projection{};
    alignas(16) glm::mat4 view{};
    alignas(16) glm::mat4 model{};
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

typedef struct Ir77PVInputBuffer {
    std::vector<Ir77PVVertex> vertex_data;
    std::vector<std::uint32_t> index_data;
}* pIr77PVInputBuffer;

typedef struct Ir77MeshRange {
    std::uint32_t index_count;
    std::uint32_t instance_count = 0;
    std::uint32_t index_offset;
    std::int32_t vertex_offset;
    std::uint32_t first_instance;
}* pIr77MeshRange;

static_assert(sizeof(Ir77MeshRange) == sizeof(VkDrawIndexedIndirectCommand));

typedef struct Ir77PVVertexSkinned { 
    glm::vec3 position{}; 
    glm::vec3 normal{}; 
    glm::vec2 uv{}; 
    glm::uvec4 joints{}; 
    glm::vec4 weights{}; 
}* pIr77PVVertexSkinned;

struct IIr77PVBuffer;
typedef struct Ir77PVDescriptorBinding {
    std::uint32_t binding{0};
    VkDescriptorType type{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
    std::shared_ptr<IIr77PVBuffer> buffer{};
    VkDescriptorImageInfo image{};
    bool is_image{false};
}* pIr77PVDescriptorBinding;

typedef struct Ir77PVFrameSizes {
    VkDeviceSize camera{256};                         // view, proj, view_proj, eye position
    VkDeviceSize lights{64 * 64};                       // 64 lights x 64 bytes
    VkDeviceSize instances{4096 * 64};            // 4096 instances x mat4
    VkDeviceSize shadow_matrices{4 * 64};     // 4 cascades / lights x mat4
    VkDeviceSize bones{256 * 128 * 64};          // 256 skeletons x 128 joints x mat4
    VkDeviceSize indirect{4096 * sizeof(VkDrawIndexedIndirectCommand)};
}* pIr77PVFrameSizes;

struct IIr77PVPipeline;
struct IIr77PVLayout;
struct IIr77PVDescriptorSet;
struct IIr77PVDescriptorSet;
typedef struct Ir77PVDrawItem {
    std::shared_ptr<IIr77PVPipeline> pipeline{};
    std::shared_ptr<IIr77PVLayout> layout{};
    std::shared_ptr<IIr77PVDescriptorSet> material{};  // set 2 -- Static / Skinned / Transparent
    std::shared_ptr<IIr77PVDescriptorSet> bones{};  // set 3 Skinned, set 1 ShadowSkinned
    std::shared_ptr<IIr77PVBuffer> vertex{};
    std::shared_ptr<IIr77PVBuffer> index{};
    VkIndexType index_type{VK_INDEX_TYPE_UINT32};
    std::uint32_t index_count{0};
    std::uint32_t first_index{0};
    std::int32_t vertex_offset{0};
    std::uint32_t instance_count{1};
    std::uint32_t first_instance{0};
    std::shared_ptr<IIr77PVBuffer> indirect{};
    VkDeviceSize indirect_offset{0};
    std::uint32_t indirect_count{0};
    std::array<float, 16> model{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    std::uint32_t light_index{0};  // shadow kinds only
}* pIr77PVDrawItem;

typedef struct Ir77PVSamplerDesc {
    VkFilter filter{VK_FILTER_LINEAR};
    VkSamplerAddressMode address{VK_SAMPLER_ADDRESS_MODE_REPEAT};
    float anisotropy{0.0f};  // 0 = off; >0 requires samplerAnisotropy enabled on the device
    bool compare{false};  // depth compare (sampler2DShadow / PCF); Depth kind only
}* pIr77PVSamplerDesc;

typedef struct Ir77PVUploadEntry {
    VkBuffer dst{VK_NULL_HANDLE};
    void const* data{nullptr};
    VkDeviceSize size{0};
}* pIr77PVUploadEntry;

}  // namespace NSIr77PeregrineV