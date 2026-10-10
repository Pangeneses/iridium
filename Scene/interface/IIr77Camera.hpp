#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fastgltf/core.hpp>
#include "fastgltf/types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../PeregrineV/types//Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

typedef struct IIr77Camera : virtual public IIr77Enlisted {
    IIr77Camera() = default;

    virtual std::shared_ptr<IIr77Return const> AddCamera(fastgltf::Camera const& camera, glm::mat4 const& world_transform) = 0;

    virtual std::shared_ptr<IIr77Return const> GetCameraCount(std::uint32_t& count) = 0;

    virtual std::shared_ptr<IIr77Return const> BuildCamera(std::uint32_t const& index, float const& aspect, float const& fallback_far,
                                                           Ir77PVCamera& camera) = 0;

    virtual ~IIr77Camera() = default;
}* pIIr77Camera;
}  // namespace NSIr77Scene
