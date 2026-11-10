#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../../PeregrineV/types/Ir77Types.hpp"

using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

// A skinned vertex's skin matrix -- the sum skinned.vert makes (weights x the instance's joint palette), before the instance's models[]
// matrix. bones = the asset's palettes back to back (Model::WriteBoneMatrices); base = where this instance's starts. A joint past the palette
// counts as no joint; no joints at all -> identity (where the file put it).
inline glm::mat4 Ir77SkinMatrix(Ir77PVVertexSkinned const& vertex, std::vector<glm::mat4> const& bones, std::uint32_t const base) {
    glm::mat4 skin{0.0f};
    float total{0.0f};

    for (int j = 0; j < 4; j++) {
        std::size_t const joint = static_cast<std::size_t>(base) + vertex.joints[j];
        if (vertex.weights[j] == 0.0f || joint >= bones.size()) continue;

        skin += vertex.weights[j] * bones[joint];
        total += vertex.weights[j];
    }

    return total > 0.0f ? skin : glm::mat4(1.0f);
}

// A skinned vertex in its instance's space -- its position through its skin matrix
inline glm::vec3 Ir77SkinPosition(Ir77PVVertexSkinned const& vertex, std::vector<glm::mat4> const& bones, std::uint32_t const base) {
    return glm::vec3(Ir77SkinMatrix(vertex, bones, base) * glm::vec4(vertex.position, 1.0f));
}

}  // namespace NSIr77Scene
