#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

typedef struct IIr77Scene : virtual public IIr77Enlisted {
    IIr77Scene() = default;

    virtual std::shared_ptr<IIr77Return const> ParseAsset(const std::string& path) = 0;

    virtual ~IIr77Scene() = default;
}* pIIr77Scene;
}  // namespace NSIr77PeregrineV
