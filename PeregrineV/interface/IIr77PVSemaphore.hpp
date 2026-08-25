#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77PVSemaphoreType : uint32_t {
    Binary = 0,    // classic signal/wait
    Timeline = 1,  // counter-based, multi-queue ordering
};

struct IIr77PVPregrineV;

typedef struct IIr77PVSemaphore : virtual public IIr77Enlisted {
    IIr77PVSemaphore() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual ~IIr77PVSemaphore() = default;
}* pIIr77PVSemaphore;
}  // namespace NSIr77PeregrineV