#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVPregrineV;

// Surface/Swapchain (optional — only for present contexts):
typedef struct IIr77PVSurface : virtual public IIr77Enlisted {
    IIr77PVSurface() = default;

    /************* RUNTIME *************/
    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSurface() = 0;

    virtual std::shared_ptr<IIr77Return const> InitSurfaceCapabilities() const = 0;

    virtual std::shared_ptr<IIr77Return const> InitPresentMode() const = 0;

    virtual std::shared_ptr<IIr77Return const> InitSurfaceFormat() const = 0;

    virtual std::shared_ptr<IIr77Return const> InitSwapChainInfo() const = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSwapchain() const = 0;

    virtual std::shared_ptr<IIr77Return const> InitSwapchainImages() const = 0;

    virtual std::shared_ptr<IIr77Return const> CreateImageView() const = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetSurface(VkSurfaceKHR* surface) const = 0;

    virtual ~IIr77PVSurface() = default;
}* PIr77PVSurface;

}  // namespace NSIr77PeregrineV

/*
    swapchain images + views
*/