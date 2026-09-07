#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <fastgltf/core.hpp>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77Scene {

typedef struct IIr77Material : virtual public IIr77Enlisted {
    IIr77Material() = default;

    virtual std::shared_ptr<IIr77Return const> LoadMaterialsFromAsset(fastgltf::Asset const& materials) = 0;

    virtual ~IIr77Material() = default;
}* pIIr77Material;
}  // namespace NSIr77Scene
