#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVLayoutSet : virtual public IIr77Enlisted {
    IIr77PVLayoutSet() = default;


    virtual ~IIr77PVLayoutSet() = default;
}* PIr77PVDescriptorSet;
}  // namespace NSIr77PeregrineV
