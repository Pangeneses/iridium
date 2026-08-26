#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVPipeline.hpp"
#include "IIr77PVRenderPass.hpp"
#include "IIr77PVSwapchain.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

enum class Ir77PVCmdBufferLevel : uint32_t {
    Primary = 0,    // submitted directly to queue
    Secondary = 1,  // executed from primary via vkCmdExecuteCommands
};

typedef struct IIr77PVCmdBuffer : virtual public IIr77Enlisted {
    IIr77PVCmdBuffer() = default;

    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    virtual std::shared_ptr<IIr77Return const> SetPipeline(std::shared_ptr<IIr77PVPipeline> pipelines) = 0;

    virtual std::shared_ptr<IIr77Return const> SetIndex(std::uint32_t const& index) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineCommandPool() = 0;

    virtual std::shared_ptr<IIr77Return const> RecordCommands() = 0;

    virtual ~IIr77PVCmdBuffer() = default;
}* pIIr77PVCmdBuffer;
}  // namespace NSIr77PeregrineV

/*
Internal — VkCommandBuffer, VkCommandPool reference, recording state tracking.
*/