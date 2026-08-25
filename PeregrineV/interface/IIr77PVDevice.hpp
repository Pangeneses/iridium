#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVInstance.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77PVQueueType : uint32_t {
    Unknown = 0,
    Graphics = 1,
    Compute = 2,
    Transfer = 3,
    Sparse = 4,
    Protected = 5,
    Encode = 6,
    Decode = 7,
};

typedef struct Ir77PVQueueFamily {
    Ir77PVQueueType type{Ir77PVQueueType::Unknown};
    VkQueueFamilyProperties family_properties;
    VkDeviceQueueCreateInfo create_info;
    VkBool32 presentation;
    VkQueue queue;
}* pIr77PVQueueFamily;

typedef struct IIr77PVDevice : virtual public IIr77Enlisted {
    IIr77PVDevice() = default;

    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineQueueFamilyProps(SDL_Window* window) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineQueueCreateInfos() = 0;

    virtual std::shared_ptr<IIr77Return const> CheckDeviceExtensionSupport() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineDeviceInfo() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateDevice() = 0;

    virtual std::shared_ptr<IIr77Return const> GetPhysicalDevice(VkPhysicalDevice* phys_device) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDeviceProperties(VkPhysicalDeviceProperties& phys_device_props) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDeviceFeatures(VkPhysicalDeviceFeatures& phys_device_features) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDevice(VkDevice* device) = 0;

    virtual std::shared_ptr<IIr77Return const> GetQueueFamily(std::vector<Ir77PVQueueFamily>& queue_family) = 0;

    virtual ~IIr77PVDevice() = default;
}* pIIr77PVDevice;
}  // namespace NSIr77PeregrineV
