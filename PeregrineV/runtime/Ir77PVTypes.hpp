#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace NSIr77PeregrineV {

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

}  // namespace NSIr77PeregrineV