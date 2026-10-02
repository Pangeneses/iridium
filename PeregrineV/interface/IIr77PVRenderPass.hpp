#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVInstance.hpp"
#include "IIr77PVDevice.hpp"

#include "../server/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVRenderPass : virtual public IIr77Enlisted {
    IIr77PVRenderPass() = default;

    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // SetKind first -- it resets formats to the kind's defaults
    virtual std::shared_ptr<IIr77Return const> SetKind(Ir77PVRenderPassKind const& kind) = 0;

    // Main / Post: must equal the swapchain's surface format. Offscreen: defaults to R16G16B16A16_SFLOAT.
    virtual std::shared_ptr<IIr77Return const> SetColorFormat(VkFormat const& format) = 0;

    // Main / Shadow / Offscreen: VK_FORMAT_UNDEFINED (default) picks the best supported depth format
    virtual std::shared_ptr<IIr77Return const> SetDepthFormat(VkFormat const& format) = 0;

    virtual std::shared_ptr<IIr77Return const> CreateRenderPass() = 0;

    virtual std::shared_ptr<IIr77Return const> GetRenderPass(VkRenderPass* render_pass) = 0;

    virtual std::shared_ptr<IIr77Return const> GetKind(Ir77PVRenderPassKind* kind) = 0;

    virtual std::shared_ptr<IIr77Return const> GetColorFormat(VkFormat* format) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDepthFormat(VkFormat* format) = 0;

    virtual std::shared_ptr<IIr77Return const> HasColor(bool* has_color) = 0;

    virtual std::shared_ptr<IIr77Return const> HasDepth(bool* has_depth) = 0;

    // Attachment count, for sizing clear values and framebuffer attachment arrays
    virtual std::shared_ptr<IIr77Return const> GetAttachmentCount(std::uint32_t* count) = 0;

    virtual ~IIr77PVRenderPass() = default;
}* pIIr77PVRenderPass;
}  // namespace NSIr77PeregrineV