#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVInstance.hpp"
#include "IIr77PVDevice.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct Ir77PVRenderPassInfo {
}* pIr77PVRenderPassInfo;

typedef struct IIr77PVRenderPass : virtual public IIr77Enlisted {
    IIr77PVRenderPass() = default;

    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineColorAttachment(SDL_Window* window) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineColorAttachmentRef() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineSubpass() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineRenderPass() = 0;

    virtual std::shared_ptr<IIr77Return const> GetRenderPass(VkRenderPass* render_pass) = 0;

    virtual ~IIr77PVRenderPass() = default;
}* pIIr77PVRenderPass;
}  // namespace NSIr77PeregrineV
