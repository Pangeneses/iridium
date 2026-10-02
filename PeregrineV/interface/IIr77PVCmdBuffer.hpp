#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVSwapchain.hpp"
#include "IIr77PVRenderPass.hpp"
#include "IIr77PVPipeline.hpp"
#include "IIr77PVPipelineCPT.hpp"
#include "IIr77PVLayout.hpp"
#include "IIr77PVLayoutCPT.hpp"
#include "IIr77PVDescriptorSet.hpp"
#include "IIr77PVOverlay.hpp"

#include "../server/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

typedef struct IIr77PVCmdBuffer : virtual public IIr77Enlisted {
    IIr77PVCmdBuffer() = default;

    // wiring
    virtual std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) = 0;

    virtual std::shared_ptr<IIr77Return const> SetRenderPass(Ir77PVRenderPassKind const& kind, std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    virtual std::shared_ptr<IIr77Return const> SetShadowTarget(VkFramebuffer const& framebuffer, VkExtent2D const& extent) = 0;

    virtual std::shared_ptr<IIr77Return const> SetGlobalSet(std::shared_ptr<IIr77PVDescriptorSet> global_set) = 0;

    virtual std::shared_ptr<IIr77Return const> SetPassSet(std::shared_ptr<IIr77PVDescriptorSet> pass_set) = 0;

    virtual std::shared_ptr<IIr77Return const> SetOverlay(std::shared_ptr<IIr77PVOverlay> overlay, std::shared_ptr<IIr77PVPipeline> pipeline,
                                                          std::shared_ptr<IIr77PVLayout> layout) = 0;

    virtual std::shared_ptr<IIr77Return const> SetClearColor(VkClearColorValue const& clear_color) = 0;

    virtual std::shared_ptr<IIr77Return const> SetComputePipeline(std::shared_ptr<IIr77PVPipelineCPT> pipeline) = 0;

    virtual std::shared_ptr<IIr77Return const> SetComputeLayout(std::shared_ptr<IIr77PVLayoutCPT> layout) = 0;

    virtual std::shared_ptr<IIr77Return const> SetComputeSets(std::vector<std::shared_ptr<IIr77PVDescriptorSet>> const& sets) = 0;

    virtual std::shared_ptr<IIr77Return const> SetComputeDispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) = 0;

    // per frame
    virtual std::shared_ptr<IIr77Return const> SetCurrentFrame(std::uint32_t const& current_frame) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDraws(Ir77PVPass const& pass, std::vector<Ir77PVDrawItem> const& draws) = 0;

    virtual std::shared_ptr<IIr77Return const> ClearDraws() = 0;

    // setup
    virtual std::shared_ptr<IIr77Return const> DefineSyncObjects() = 0;

    virtual std::shared_ptr<IIr77Return const> DefineCommandPool() = 0;

    virtual std::shared_ptr<IIr77Return const> AllocateCommandBuffer() = 0;

    // frame loop
    virtual std::shared_ptr<IIr77Return const> WaitForFence() = 0;

    virtual std::shared_ptr<IIr77Return const> WaitAllFrames() = 0;

    virtual std::shared_ptr<IIr77Return const> AcquireNextImage(VkResult* acquire_next_result) = 0;

    virtual std::shared_ptr<IIr77Return const> ResetFence() = 0;

    virtual std::shared_ptr<IIr77Return const> RecordCommandBuffer() = 0;

    virtual std::shared_ptr<IIr77Return const> SubmitFrame() = 0;

    virtual std::shared_ptr<IIr77Return const> PresentFrame(VkResult* queue_present_result) = 0;

    // queries
    virtual std::shared_ptr<IIr77Return const> GetCurrentFrame(std::uint32_t* current_frame) = 0;

    virtual std::shared_ptr<IIr77Return const> GetImageIndex(std::uint32_t* image_index) = 0;

    virtual std::shared_ptr<IIr77Return const> GetCommandPool(VkCommandPool* pool) = 0;

    virtual std::shared_ptr<IIr77Return const> GetQueue(VkQueue* queue) = 0;

    virtual ~IIr77PVCmdBuffer() = default;
}* pIIr77PVCmdBuffer;
}  // namespace NSIr77PeregrineV