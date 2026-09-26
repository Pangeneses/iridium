#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <memory>

namespace NSIr77PeregrineV {
static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 3;

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


}  // namespace NSIr77PeregrineV