#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVInstance : virtual public IIr77Enlisted {
    IIr77PVInstance() = default;

    /************* RUNTIME *************/
    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> InitAppInfo() = 0;

    virtual std::shared_ptr<IIr77Return const> InitExtensions() = 0;

    virtual std::shared_ptr<IIr77Return const> InitCreateInfo() = 0;

    virtual std::shared_ptr<IIr77Return const> InitCreateInstance() = 0;

    virtual std::shared_ptr<IIr77Return const> QueryDeviceCount(std::uint32_t const& count) = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetVkInstance(VkInstance* instance) = 0;

    virtual ~IIr77PVInstance() = default;
}* PIr77PVInstance;
}  // namespace NSIr77PeregrineV