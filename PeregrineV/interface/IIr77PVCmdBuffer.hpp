#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVSwapchain.hpp"
#include "IIr77PVRenderPass.hpp"
#include "IIr77PVLayout.hpp"
#include "IIr77PVDescriptorSet.hpp"
#include "IIr77PVPipeline.hpp"
#include "IIr77PVPipelineCPT.hpp"
#include "IIr77PVPipelineCache.hpp"
#include "IIr77PVOverlay.hpp"
#include "IIr77PVAcceleration.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// One window's frame loop: command pool and buffers, sync objects, recording, submit and present.
// Bindless: the one descriptor set is bound once per bind point per command buffer; every draw / dispatch gets Ir77PVPushConstants.
// Dynamic rendering: passes begin on the swapchain / shadow images directly; cull, front face and depth test / write are set per draw.
typedef struct IIr77PVCmdBuffer : virtual public IIr77Enlisted {
    IIr77PVCmdBuffer() = default;

    // ---- configuration ---------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    virtual std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) = 0;

    // Attachment formats of one pass kind -- the pass itself is begun with vkCmdBeginRendering
    virtual std::shared_ptr<IIr77Return const> SetRenderPass(Ir77PVRenderPassKind const& kind, std::shared_ptr<IIr77PVRenderPass> render_pass) = 0;

    // Depth image the Shadow / ShadowSkinned pipelines render into (in the Shadow pass's depth format); the main pass samples it afterwards
    virtual std::shared_ptr<IIr77Return const> SetShadowTarget(VkImage const& image, VkImageView const& view, VkExtent2D const& extent) = 0;

    // The device's one pipeline layout -- push constants and the set bind go through it
    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) = 0;

    // The device's one bindless table
    virtual std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) = 0;

    // This window's frame table slot per view + the slots they list, one entry per frame in flight
    virtual std::shared_ptr<IIr77Return const> SetFrameSlots(std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT> const& frame_slots) = 0;

    // Buffer slot of the device's material table (Ir77PeregrineV::GetMaterialTableSlot)
    virtual std::shared_ptr<IIr77Return const> SetMaterialTable(std::uint32_t const& slot) = 0;

    // The device's pipeline cache -- resolves the pipeline handles on draw items and the overlay while recording. Required.
    virtual std::shared_ptr<IIr77Return const> SetPipelineCache(std::shared_ptr<IIr77PVPipelineCache> pipeline_cache) = 0;

    // CEF composite, drawn by the pass that has overlay set; the overlay's texture slot goes in pc.user[0]. pipeline -- usually FindPipeline(CEF).
    virtual std::shared_ptr<IIr77Return const> SetOverlay(std::shared_ptr<IIr77PVOverlay> overlay, Ir77PVPipelineHandle const& pipeline) = 0;

    virtual std::shared_ptr<IIr77Return const> SetClearColor(VkClearColorValue const& clear_color) = 0;

    // ---- pass list -----------------------------------------------------------------------------------------------------------------------
    // What RecordCommandBuffer records, in order. At least one Raster pass must target the swapchain -- the first clears it, the last presents.
    // Default: Compute -> Shadow -> Main (view 0, the whole window).
    virtual std::shared_ptr<IIr77Return const> SetPassList(std::vector<Ir77PVPassDesc> const& passes) = 0;

    virtual std::shared_ptr<IIr77Return const> GetPassList(std::vector<Ir77PVPassDesc>& passes) = 0;

    // ---- compute -----------------------------------------------------------------------------------------------------------------------------
    // The dispatch list, replaced whole; recorded in order by the Compute pass, a barrier after each. Empty = no compute work.
    virtual std::shared_ptr<IIr77Return const> SetDispatches(std::vector<Ir77PVDispatch> const& dispatches) = 0;

    // Recorded by the Compute pass once per view the pass list draws, after the dispatch list and the acceleration build -- each with that
    // view's frame table (pc.frame) and pc.user[7] = the view. The cull pass. Empty = none.
    virtual std::shared_ptr<IIr77Return const> SetViewDispatches(std::vector<Ir77PVDispatch> const& dispatches) = 0;

    // ---- acceleration structures -------------------------------------------------------------------------------------------------------------
    // Built right after the Compute pass (BLAS refits, TLAS). Paint sets it each frame on the first window it records. Unset = no builds.
    virtual std::shared_ptr<IIr77Return const> SetAcceleration(std::shared_ptr<IIr77PVAcceleration> acceleration) = 0;

    /*
    // ---- ray tracing -------------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> SetRayTracing(std::shared_ptr<IIr77PVPipelineRT> pipeline, Ir77PVPushConstants const& push) = 0;

    virtual std::shared_ptr<IIr77Return const> SetRayTarget(VkImage const& image, VkExtent2D const& extent) = 0;

    virtual std::shared_ptr<IIr77Return const> SetAccelStructure(std::shared_ptr<IIr77PVAccelStructure> tlas) = 0;
    */

    // ---- create ------------------------------------------------------------------------------------------------------------------------------
    // Pool, per-frame command buffers, per-frame sync objects and extension commands. After SetDevice.
    virtual std::shared_ptr<IIr77Return const> CreateCommandBuffer() = 0;

    // ---- per frame: SetCurrentFrame -> WaitForFence -> AcquireNextImage -> ResetFence -> RecordCommandBuffer -> SubmitFrame -> PresentFrame
    virtual std::shared_ptr<IIr77Return const> SetCurrentFrame(std::uint32_t const& current_frame) = 0;

    // Replaces one view's list for one pass. Opaque sorted by pipeline then material; Transparent back to front from that view's eye.
    virtual std::shared_ptr<IIr77Return const> SetDraws(std::uint32_t const& view, Ir77PVPass const& pass, std::vector<Ir77PVDrawItem> const& draws) = 0;

    virtual std::shared_ptr<IIr77Return const> ClearDraws() = 0;

    virtual std::shared_ptr<IIr77Return const> WaitForFence() = 0;

    // Every frame-in-flight fence of this window, instead of vkDeviceWaitIdle
    virtual std::shared_ptr<IIr77Return const> WaitAllFrames() = 0;

    // SUBOPTIMAL and OUT_OF_DATE are handed back in acquire_next_result for the caller to rebuild; anything else is an error
    virtual std::shared_ptr<IIr77Return const> AcquireNextImage(VkResult* acquire_next_result) = 0;

    // Only after a successful acquire
    virtual std::shared_ptr<IIr77Return const> ResetFence() = 0;

    virtual std::shared_ptr<IIr77Return const> RecordCommandBuffer() = 0;

    virtual std::shared_ptr<IIr77Return const> SubmitFrame() = 0;

    // SUBOPTIMAL and OUT_OF_DATE are handed back in queue_present_result for the caller to rebuild
    virtual std::shared_ptr<IIr77Return const> PresentFrame(VkResult* queue_present_result) = 0;

    // ---- queries -----------------------------------------------------------------------------------------------------------------------------
    virtual std::shared_ptr<IIr77Return const> GetCurrentFrame(std::uint32_t* current_frame) = 0;

    virtual std::shared_ptr<IIr77Return const> GetImageIndex(std::uint32_t* image_index) = 0;

    virtual std::shared_ptr<IIr77Return const> GetCommandPool(VkCommandPool* pool) = 0;

    virtual std::shared_ptr<IIr77Return const> GetQueue(VkQueue* queue) = 0;

    virtual ~IIr77PVCmdBuffer() = default;
}* pIIr77PVCmdBuffer;
}  // namespace NSIr77PeregrineV
