#pragma once

#include <SDL3/SDL_video.h>

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVInstance.hpp"
#include "IIr77PVDevice.hpp"
#include "IIr77PVRenderPass.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// One window's surface, swapchain, image views and shared depth image. The Main pass renders into them directly (dynamic rendering).
typedef struct IIr77PVSwapchain : virtual public IIr77Enlisted {
    IIr77PVSwapchain() = default;

    // ---- configuration -----------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // the depth image needs it
    virtual std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) = 0;

    // the Main pass -- its depth format sizes the depth image
    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateSurface(SDL_Window* window) = 0;

    // ---- build / rebuild ---------------------------------------------------------------------------------------------------------------------
    // first build -- support, format, present mode, extent, swapchain, images, views, depth image
    virtual std::shared_ptr<IIr77Return const> CreateSwapchain() = 0;

    // after OUT_OF_DATE / SUBOPTIMAL -- the same sequence, releasing what the new swapchain replaced
    virtual std::shared_ptr<IIr77Return const> RebuildSwapchain() = 0;

    // surface format only -- for render passes created before the swapchain
    virtual std::shared_ptr<IIr77Return const> QuerySurfaceFormat(VkSurfaceFormatKHR& surface_format) = 0;

    // ---- queries -----------------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> GetSwapchain(VkSwapchainKHR* swapchain) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSwapchainExtents(VkExtent2D& swapchain_extent) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSwapchainImages(std::vector<VkImage>& swapchain_images) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSwapchainViews(std::vector<VkImageView>& swapchain_views) = 0;

    // the shared depth image + view -- the Main pass's depth attachment
    virtual std::shared_ptr<IIr77Return const> GetDepthTarget(VkImage& image, VkImageView& view) = 0;

    virtual std::shared_ptr<IIr77Return const> GetSurfaceFormat(VkSurfaceFormatKHR& surface_format) = 0;

    virtual ~IIr77PVSwapchain() = default;
}* pIIr77PVSwapchain;
}  // namespace NSIr77PeregrineV
