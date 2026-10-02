#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVInstance.hpp"
#include "IIr77PVDevice.hpp"
#include "IIr77PVRenderPass.hpp"

#include "../server/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVSwapchain : virtual public IIr77Enlisted {
    IIr77PVSwapchain() = default;

    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSurface(SDL_Window* window) = 0;

    virtual std::shared_ptr<IIr77Return const> CleanupSwapchain() = 0;

    virtual std::shared_ptr<IIr77Return const> QuerySwapchainSupport() = 0;

    virtual std::shared_ptr<IIr77Return const> SwapSurfaceFormat() = 0;

    virtual std::shared_ptr<IIr77Return const> PresentMode() = 0;

    virtual std::shared_ptr<IIr77Return const> SurfaceCapabilities() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineSwapchain() = 0;

    virtual std::shared_ptr<IIr77Return const> InitSwapchainImages() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineImageView() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineFramebuffers() = 0;

    virtual std::shared_ptr<IIr77Return const> GetSwapchain(VkSwapchainKHR* swapchain) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSwapchainExtents(VkExtent2D& swapchain_extent) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSwapchainViews(std::vector<VkImageView>& swapchain_views) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSwapchainFramebuffers(std::vector<VkFramebuffer>& swapchain_framebuffers) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSurfaceFormat(VkSurfaceFormatKHR& surface_format) = 0;

    virtual ~IIr77PVSwapchain() = default;
}* pIIr77PVSwapchain;

}  // namespace NSIr77PeregrineV
