#pragma once
#include <SDL3/SDL_stdinc.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PeregrineV.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVQueue : virtual public IIr77Enlisted {
    IIr77PVQueue() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context, std::uint32_t const& device_index) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateQueues() = 0;

    virtual std::shared_ptr<IIr77Return const> GetQueueInfo(Ir77PVQueueInfo& queue_info) = 0;

    virtual ~IIr77PVQueue() = default;
}* PIr77PVQueue;
}  // namespace NSIr77PeregrineV