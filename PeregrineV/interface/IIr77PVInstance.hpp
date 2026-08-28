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

    virtual std::shared_ptr<IIr77Return const> DefineAppInfo() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineExtensions() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineCreateInfo() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineCreateInstance() = 0;

    virtual std::shared_ptr<IIr77Return const> QueryDeviceCount(std::uint32_t& count) = 0;

    virtual std::shared_ptr<IIr77Return const> GetInstance(VkInstance* instance) = 0;

    virtual ~IIr77PVInstance() = default;
}* pIIr77PVInstance;
}  // namespace NSIr77PeregrineV