#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct Ir77PVDeviceInfo {
    VkPhysicalDevice phys_device;
    VkDevice device;
    std::vector<VkExtensionProperties> available_extensions;
    VkPhysicalDeviceFeatures phys_device_features;
    VkPhysicalDeviceLimits phys_device_limits;
    VkDeviceCreateInfo device_create_info;
};

// Device-level (one per physical GPU):
typedef struct IIr77PVDevice : virtual public IIr77Enlisted {
    IIr77PVDevice() = default;

    /************* RUNTIME *************/
    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateDevices() = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetDeviceInfos(std::vector<Ir77PVDeviceInfo>& device_info) = 0;

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