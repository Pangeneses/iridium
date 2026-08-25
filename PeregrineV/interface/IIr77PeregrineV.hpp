#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <map>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct Ir77PVEdge {
    std::uint64_t node_a_id;
    std::shared_ptr<IIr77Enlisted> node_a;
    std::uint64_t node_b_id;
    std::shared_ptr<IIr77Enlisted> node_b;
}* pIr77PVEdge;

typedef struct IIr77PeregrineV : virtual public IIr77Enlisted {
    IIr77PeregrineV() = default;

    virtual std::shared_ptr<IIr77Return const> CreateInstance(std::uint32_t& count) = 0;

    virtual std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSurfaces() = 0;

    virtual std::shared_ptr<IIr77Return const> EnumerateDeviceQueues() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateLogicalDevices() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateLayout() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateRenderPass() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSwapchains() = 0;

    virtual ~IIr77PeregrineV() = default;
}* pIIr77PeregrineV;

}  // namespace NSIr77PeregrineV