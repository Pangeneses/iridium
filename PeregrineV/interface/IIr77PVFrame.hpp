#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <cstring>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

struct IIr77PVCommandBuffer;
struct IIr77PVSemaphore;
struct IIr77PVRenderTarget;
struct IIr77PVDepthTarget;
struct IIr77PVQueue;

typedef struct IIr77PVFrame : virtual public IIr77Enlisted {
    IIr77PVFrame() = default;

    virtual std::shared_ptr<IIr77Return const> SetIndex(uint32_t index) = 0;

    virtual std::shared_ptr<IIr77Return const> GetIndex(uint32_t& index) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetCommandBuffer(std::shared_ptr<IIr77PVCommandBuffer const>& cmd) = 0;

    virtual std::shared_ptr<IIr77Return const> GetCommandBuffer(std::shared_ptr<IIr77PVCommandBuffer const>& cmd) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetImageAvailable(std::shared_ptr<IIr77PVSemaphore const>& semaphore) = 0;

    virtual std::shared_ptr<IIr77Return const> GetImageAvailable(std::shared_ptr<IIr77PVSemaphore const>& semaphore) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderFinished(std::shared_ptr<IIr77PVSemaphore const>& semaphore) = 0;

    virtual std::shared_ptr<IIr77Return const> GetRenderFinished(std::shared_ptr<IIr77PVSemaphore const>& semaphore) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderTarget(std::shared_ptr<IIr77PVRenderTarget const>& target) = 0;

    virtual std::shared_ptr<IIr77Return const> GetRenderTarget(std::shared_ptr<IIr77PVRenderTarget const>& target) const = 0;

    virtual std::shared_ptr<IIr77Return const> SetDepthTarget(std::shared_ptr<IIr77PVDepthTarget const>& target) = 0;

    virtual std::shared_ptr<IIr77Return const> GetDepthTarget(std::shared_ptr<IIr77PVDepthTarget const>& target) const = 0;

    virtual std::shared_ptr<IIr77Return const> Begin() = 0;

    virtual std::shared_ptr<IIr77Return const> End() = 0;

    virtual std::shared_ptr<IIr77Return const> Submit(std::shared_ptr<IIr77PVQueue const>& queue) = 0;

    virtual std::shared_ptr<IIr77Return const> Present(std::shared_ptr<IIr77PVQueue const>& queue) = 0;

    virtual std::shared_ptr<IIr77Return const> WaitIdle() = 0;

    virtual std::shared_ptr<IIr77Return const> Build() = 0;

    virtual ~IIr77PVFrame() = default;
}* PIr77PVFrame;
}  // namespace NSIr77REDOS

/*
Internal — VkFence for CPU/GPU sync, swapchain image index, frame-in-flight state.
Begin waits on the fence + acquires the swapchain image + begins the command buffer. End ends the command buffer. Submit signals RenderFinished, waits on
ImageAvailable. Present waits on RenderFinished + calls vkQueuePresentKHR. WaitIdle stalls CPU until the frame fence signals — used for clean shutdown or
resize.
*/