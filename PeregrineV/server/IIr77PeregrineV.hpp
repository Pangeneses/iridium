#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <map>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVDevice;

typedef struct IIr77PeregrineV : virtual public IIr77Enlisted {
    IIr77PeregrineV() = default;

    virtual std::shared_ptr<IIr77Return const> SetCurrentDevice(std::uint64_t const& device_id) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateInstance() = 0;

    virtual std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices(std::map<std::uint64_t, std::shared_ptr<IIr77PVDevice>>& devices) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSurfaces(std::map<std::uint64_t, std::vector<SDL_Window*>> const& windows) = 0;

    virtual std::shared_ptr<IIr77Return const> EnumerateDeviceQueues() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateLogicalDevices() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateLayout() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateRenderPass() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSwapchains() = 0;

    virtual std::shared_ptr<IIr77Return const> CreatePipelineGFX() = 0;

    virtual ~IIr77PeregrineV() = default;
}* pIIr77PeregrineV;

}  // namespace NSIr77PeregrineV