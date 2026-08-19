#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVPregrineV;
struct IIr77PVQueue;

// Device-level (one per physical GPU):
typedef struct IIr77PVDevice : virtual public IIr77Enlisted {
    IIr77PVDevice() = default;

    /************* RUNTIME *************/
    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> EnumeratePhysicalDevice() = 0;

    virtual std::shared_ptr<IIr77Return const> CheckDeviceExtensionSupport(std::uint32_t const& index) = 0;

    virtual std::shared_ptr<IIr77Return const> InitCreateDeviceInfo() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateDevice(std::uint32_t const& index) = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetVkPhysicalDevice(VkPhysicalDevice* phys_device, std::uint32_t const& index) = 0;

    virtual std::shared_ptr<IIr77Return const> GetVkDevice(VkDevice* device) = 0;

    virtual ~IIr77PVDevice() = default;
}* PIr77PVDevice;
}  // namespace NSIr77PeregrineV

/*
IIr77PVDevice
    device properties / features / limits
    vendor ID / device name
    memory properties
    queue family indices(graphics, compute, transfer, present)
    VkQueue per family
*/