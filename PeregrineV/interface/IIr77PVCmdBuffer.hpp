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

struct Ir77PVBufferVertex;
struct Ir77PVBufferUBO;
struct Ir77PVBufferCEF;

typedef struct IIr77PVCmdBuffer : virtual public IIr77Enlisted {
    IIr77PVCmdBuffer() = default;

    virtual std::shared_ptr<IIr77Return const> SetCurrentFrame(std::uint32_t const& current_frame) = 0;

    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    virtual std::shared_ptr<IIr77Return const> SetPipelineGFX(std::shared_ptr<IIr77PVPipeline> pipeline_gfx) = 0;

    virtual std::shared_ptr<IIr77Return const> SetBufferVertex(std::shared_ptr<Ir77PVBufferVertex> buffer_vertex) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayoutUBO(std::shared_ptr<IIr77PVLayout> layout_ubo) = 0;

    virtual std::shared_ptr<IIr77Return const> SetBufferUBO(std::shared_ptr<Ir77PVBufferUBO> buffer_ubo) = 0;

    virtual std::shared_ptr<IIr77Return const> SetPipelineCEF(std::shared_ptr<IIr77PVPipeline> pipeline_cef) = 0;

    virtual std::shared_ptr<IIr77Return const> SetLayoutCEF(std::shared_ptr<IIr77PVLayout> layout_cef) = 0;

    virtual std::shared_ptr<IIr77Return const> SetBufferCEF(std::shared_ptr<Ir77PVBufferCEF> buffer_cef) = 0;

    virtual std::shared_ptr<IIr77Return const> DefineSyncObjects() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineCommandPool() = 0;

    virtual std::shared_ptr<IIr77Return const> AllocateCommandBuffer() = 0;

    virtual std::shared_ptr<IIr77Return const> WaitForFence() = 0;

    virtual std::shared_ptr<IIr77Return const> ResetFence() = 0;

    virtual std::shared_ptr<IIr77Return const> AcquireNextImage(VkResult* acquire_next_result) = 0;

    virtual std::shared_ptr<IIr77Return const> RecordCommandBuffer() = 0;

    virtual std::shared_ptr<IIr77Return const> SubmitFrame() = 0;

    virtual std::shared_ptr<IIr77Return const> PresentFrame(VkResult* queue_present_result) = 0;

    virtual ~IIr77PVCmdBuffer() = default;
}* pIIr77PVCmdBuffer;
}  // namespace NSIr77PeregrineV

/*
Internal — VkCommandBuffer, VkCommandPool reference, recording state tracking.
*/