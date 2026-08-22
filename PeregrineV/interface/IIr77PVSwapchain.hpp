#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PeregrineV.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Surface/Swapchain (optional — only for present contexts):
typedef struct IIr77PVSwapchain : virtual public IIr77Enlisted {
    IIr77PVSwapchain() = default;

    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context, std::uint32_t const& device_index) = 0;

    virtual std::shared_ptr<IIr77Return const> BindWindow() = 0;

    virtual std::shared_ptr<IIr77Return const> GetSwapchainInfos(std::vector<Ir77PVSwapchainInfo>& swapchain_infos) = 0;

    virtual ~IIr77PVSwapchain() = default;
}* PIr77PVSwapchain;

}  // namespace NSIr77PeregrineV

/*
    swapchain images + views
*/