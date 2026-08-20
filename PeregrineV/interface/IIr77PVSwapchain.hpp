#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct SwapchainSupportDetails {
    VkPhysicalDeviceProperties device_properties;
    std::vector<VkSurfaceFormatKHR> formats;
    VkFormat format{VK_FORMAT_B8G8R8A8_SRGB};
    VkColorSpaceKHR color_space{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    std::vector<VkPresentModeKHR> present_modes;
    VkPresentModeKHR present_mode{VK_PRESENT_MODE_MAILBOX_KHR};
    VkSurfaceCapabilitiesKHR capabilities;
    VkExtent2D swapchain_extent{};
    std::uint32_t imageCount{0};
};

struct IIr77PVPregrineV;

// Surface/Swapchain (optional — only for present contexts):
typedef struct IIr77PVSwapchain : virtual public IIr77Enlisted {
    IIr77PVSwapchain() = default;

    /************* RUNTIME *************/
    virtual std::shared_ptr<IIr77Return const> Initialize(std::shared_ptr<IIr77Enlisted>& context) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSurface() = 0;

    virtual std::shared_ptr<IIr77Return const> QuerySwapchainSupport(SwapchainSupportDetails& details) = 0;

    virtual std::shared_ptr<IIr77Return const> SwapSurfaceFormat() = 0;

    virtual std::shared_ptr<IIr77Return const> PresentMode(bool& available) = 0;

    virtual std::shared_ptr<IIr77Return const> SurfaceCapabilities() = 0;

    virtual std::shared_ptr<IIr77Return const> InitSwapchainInfo() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSwapchain() = 0;

    virtual std::shared_ptr<IIr77Return const> InitSwapchainImages() = 0;

    virtual std::shared_ptr<IIr77Return const> CreateImageView() = 0;

    virtual std::shared_ptr<IIr77Return const> GetImage(VkImage* image) = 0;

    /************* GETTERS *************/
    virtual std::shared_ptr<IIr77Return const> GetSurface(VkSurfaceKHR* surface) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSwapchainSupportDetails(SwapchainSupportDetails& details) = 0;

    virtual ~IIr77PVSwapchain() = default;
}* PIr77PVSwapchain;

}  // namespace NSIr77PeregrineV

/*
    swapchain images + views
*/