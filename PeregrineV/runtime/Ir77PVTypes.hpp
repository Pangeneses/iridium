#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace NSIr77PeregrineV {
static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 3;

struct Ir77PVVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Ir77PVCameraUBO {
    alignas(16) glm::mat4 projection;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 model;
};

typedef struct Ir77PVInputBuffer {
    std::vector<Ir77PVVertex> vertex_data; 
    std::vector<std::uint32_t> index_data;
}* pIr77PVInputBuffer;

}  // namespace NSIr77PeregrineV