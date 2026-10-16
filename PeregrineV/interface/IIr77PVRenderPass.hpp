#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// What one kind of pass renders into, for dynamic rendering -- color / depth formats only, no VkRenderPass and no framebuffers.
// Pipelines declare these formats; the swapchain and the shadow target size their depth images with GetDepthFormat.
typedef struct IIr77PVRenderPass : virtual public IIr77Enlisted {
    IIr77PVRenderPass() = default;

    // ---- configuration: SetDevice -> SetKind (resets the formats) -> [SetColorFormat / SetDepthFormat] -------------------------------------
    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetKind(Ir77PVRenderPassKind const& kind) = 0;

    virtual std::shared_ptr<IIr77Return const> SetColorFormat(VkFormat const& format) = 0;

    // Optional -- left UNDEFINED, CreateRenderPass picks the first supported depth format
    virtual std::shared_ptr<IIr77Return const> SetDepthFormat(VkFormat const& format) = 0;

    // ---- create ------------------------------------------------------------------------------------------------------------------------------
    // Validates the color format and resolves the depth format
    virtual std::shared_ptr<IIr77Return const> CreateRenderPass() = 0;

    // ---- queries -----------------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> GetKind(Ir77PVRenderPassKind* kind) = 0;

    // UNDEFINED when the kind has no color attachment (Shadow)
    virtual std::shared_ptr<IIr77Return const> GetColorFormat(VkFormat* format) = 0;

    // UNDEFINED when the kind has no depth attachment (Post); valid after CreateRenderPass
    virtual std::shared_ptr<IIr77Return const> GetDepthFormat(VkFormat* format) = 0;

    virtual std::shared_ptr<IIr77Return const> HasColor(bool* has_color) = 0;

    virtual std::shared_ptr<IIr77Return const> HasDepth(bool* has_depth) = 0;

    virtual std::shared_ptr<IIr77Return const> GetAttachmentCount(std::uint32_t* count) = 0;

    virtual ~IIr77PVRenderPass() = default;
}* pIIr77PVRenderPass;
}  // namespace NSIr77PeregrineV
