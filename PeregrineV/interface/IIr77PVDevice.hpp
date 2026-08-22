#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../interface/IIr77PeregrineV.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVDevice : virtual public IIr77Enlisted {
    IIr77PVDevice() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context, std::uint32_t const& device_index) = 0;

    virtual std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices() = 0;

    virtual std::shared_ptr<IIr77Return const> CheckDeviceExtensionSupport() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineDeviceInfo() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateDevice() = 0;

    virtual std::shared_ptr<IIr77Return const> GetDeviceInfo(Ir77PVDeviceInfo& device_info) = 0;

    virtual ~IIr77PVDevice() = default;
}* PIr77PVDevice;
}  // namespace NSIr77PeregrineV
