#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fastgltf/core.hpp>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../PeregrineV/server/Ir77PVTypes.hpp"

using namespace NSIr77RT;
using namespace NSIr77PeregrineV;

namespace NSIr77Scene {

typedef struct IIr77Light : virtual public IIr77Enlisted {
    IIr77Light() = default;

    virtual std::shared_ptr<IIr77Return const> AddLight(fastgltf::Light const& light, glm::mat4 const& world_transform) = 0;

    virtual std::shared_ptr<IIr77Return const> GetLightsBuffer(std::vector<Ir77PVLight>& lights) = 0;

    virtual ~IIr77Light() = default;
}* pIIr77Light;
}  // namespace NSIr77Scene
