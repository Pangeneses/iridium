#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fastgltf/core.hpp>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"
#include "fastgltf/types.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

typedef struct IIr77Camera : virtual public IIr77Enlisted {
    IIr77Camera() = default;

    virtual std::shared_ptr<IIr77Return const> AddCamera(fastgltf::Camera const& camera, glm::mat4 const& world_transform) = 0;

    virtual ~IIr77Camera() = default;
}* pIIr77Camera;
}  // namespace NSIr77PeregrineV
