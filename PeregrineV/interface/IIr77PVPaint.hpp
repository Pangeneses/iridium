#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL_video.h>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "IIr77PVDevice.hpp"
#include "IIr77PVSwapchain.hpp"
#include "IIr77PVRenderPass.hpp"
#include "IIr77PVLayout.hpp"
#include "IIr77PVDescriptorSet.hpp"
#include "IIr77PVPipelineCache.hpp"
#include "IIr77PVTexture.hpp"
#include "IIr77PVBuffer.hpp"
#include "IIr77PVOverlay.hpp"
#include "IIr77PVAcceleration.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// Records and presents every window of one device: one command buffer per window, fed per frame with frame data, draw lists, dispatches
// and pass lists. Inputs are safe to set any time -- applied inside Draw once the frame's fence has signalled.
//   Ir77PVInit wires it: the Set* below -> CreateCommandBuffers (after content is loaded)
typedef struct IIr77PVPaint : virtual public IIr77Enlisted {
    IIr77PVPaint() = default;

    // ---- wiring ----------------------------------------------------------------------------------------------------------------------------

    virtual std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) = 0;

    // The SDL windows, in window order -- minimized windows are skipped
    virtual std::shared_ptr<IIr77Return const> SetWindows(std::vector<SDL_Window*> const& windows) = 0;

    // One per window, in window order
    virtual std::shared_ptr<IIr77Return const> SetSwapchains(std::vector<std::shared_ptr<IIr77PVSwapchain>> const& swapchains) = 0;

    // Every render pass of the device -- Main is required
    virtual std::shared_ptr<IIr77Return const> SetRenderPasses(std::map<Ir77PVRenderPassKind, std::shared_ptr<IIr77PVRenderPass>> const& render_passes) = 0;

    // The device's bindless layout + table
    virtual std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) = 0;

    virtual std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) = 0;

    // Bindless slot of the material table -> every command buffer's push constants
    virtual std::shared_ptr<IIr77Return const> SetMaterialTableSlot(std::uint32_t const& slot) = 0;

    virtual std::shared_ptr<IIr77Return const> SetPipelineCache(std::shared_ptr<IIr77PVPipelineCache> pipeline_cache) = 0;

    // The shadow atlas -- without it the Shadow pass has no target and is skipped
    virtual std::shared_ptr<IIr77Return const> SetShadowMap(std::shared_ptr<IIr77PVTexture> shadow_map) = 0;

    // [window][frame in flight] -- each view's frame table slot and the slots they list
    virtual std::shared_ptr<IIr77Return const> SetFrameSlots(std::vector<std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT>> const& frame_slots) = 0;

    // [window][slot] -- the frame-level buffers SetFrameData writes into
    virtual std::shared_ptr<IIr77Return const> SetFrameBuffers(std::vector<std::map<Ir77PVBufferSlot, std::shared_ptr<IIr77PVBuffer>>> const& frame_buffers) = 0;

    // One per window, in window order -- optional; empty = no overlay drawn
    virtual std::shared_ptr<IIr77Return const> SetOverlays(std::vector<std::shared_ptr<IIr77PVOverlay>> const& overlays) = 0;

    // Ray-query devices only -- the first window recorded each frame builds the acceleration structures after its Compute pass
    virtual std::shared_ptr<IIr77Return const> SetAcceleration(std::shared_ptr<IIr77PVAcceleration> acceleration) = 0;

    // [window] -- the frame table buffers, one table per view VIEW_STRIDE apart. Each window's copy is rewritten before submit with the TLAS
    // built this frame.
    virtual std::shared_ptr<IIr77Return const> SetFrameTables(std::vector<std::shared_ptr<IIr77PVBuffer>> const& frame_tables) = 0;

    // [frame in flight] -- Geometry's command pool slots; every frame table names its frame's (the cull pass reads it)
    virtual std::shared_ptr<IIr77Return const> SetCommandSlots(std::array<std::uint32_t, MAX_FRAMES_IN_FLIGHT> const& slots) = 0;

    // One command buffer per window. After the wiring and the pipeline cache.
    virtual std::shared_ptr<IIr77Return const> CreateCommandBuffers() = 0;

    // ---- per frame --------------------------------------------------------------------------------------------------------------------------

    // Latest contents of a frame-level buffer (camera, lights, instances ...) -- re-applied every frame to every frame-in-flight copy
    virtual std::shared_ptr<IIr77Return const> SetFrameData(std::size_t const& window, Ir77PVBufferSlot const& slot, void const* data,
                                                            VkDeviceSize const& size, VkDeviceSize const& offset = 0) = 0;

    // Replaces one view's draw list for a pass in a window. Persists until replaced.
    virtual std::shared_ptr<IIr77Return const> SetDraws(std::size_t const& window, std::uint32_t const& view, Ir77PVPass const& pass,
                                                        std::vector<Ir77PVDrawItem> const& draws) = 0;

    // Replaces the once-a-frame compute list (skinning, the field) -- recorded by the first window Draw records. Persists until replaced.
    virtual std::shared_ptr<IIr77Return const> SetFrameDispatches(std::vector<Ir77PVDispatch> const& dispatches) = 0;

    // Replaces the per-view compute list (the cull pass) -- every window, once per view it draws. Persists until replaced.
    virtual std::shared_ptr<IIr77Return const> SetViewDispatches(std::vector<Ir77PVDispatch> const& dispatches) = 0;

    // Replaces a window's pass list. At least one pass must target the swapchain.
    virtual std::shared_ptr<IIr77Return const> SetPassList(std::size_t const& window, std::vector<Ir77PVPassDesc> const& passes) = 0;

    virtual std::shared_ptr<IIr77Return const> ClearDraws(std::size_t const& window) = 0;

    // A window's background -- what its first swapchain pass clears to. Linear; persists until replaced.
    virtual std::shared_ptr<IIr77Return const> SetClearColor(std::size_t const& window, VkClearColorValue const& clear_color) = 0;

    // Records, submits and presents every window that isn't minimized
    virtual std::shared_ptr<IIr77Return const> Draw() = 0;

    // Rebuilds one window's swapchain -- the overlay and Draw pause around it
    virtual std::shared_ptr<IIr77Return const> ResetSwapchain(std::uint32_t const& index) = 0;

    virtual ~IIr77PVPaint() = default;
}* pIIr77PVPaint;
}  // namespace NSIr77PeregrineV
