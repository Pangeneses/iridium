#pragma once

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVBuffer.hpp"
#include "../interface/IIr77PVAcceleration.hpp"
#include "../interface/IIr77PVCmdBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVCmdBuffer -- one window's frame loop: command pool and buffers, sync objects, recording, submit and present.
//
//   configure: SetDevice / SetSwapchain / SetRenderPass / SetShadowTarget / SetLayout / SetDescriptorSet / SetFrameSlots / SetMaterialTable
//              / SetPipelineCache / SetOverlay / SetClearColor / [SetPassList]
//   per frame: SetDraws (per draw list) / SetDispatches (the compute list) -- any time, recorded by the next RecordCommandBuffer
//
// Bindless: the one descriptor set is bound once per bind point per command buffer; every draw / dispatch gets Ir77PVPushConstants.
//   create:    CreateCommandBuffer -> DefineExtensionFunctions -> DispatchOperationPool -> DispatchOperationBuffers -> DefineSyncObjects
//   per frame: SetCurrentFrame -> WaitForFence -> AcquireNextImage -> ResetFence -> RecordCommandBuffer -> SubmitFrame -> PresentFrame
//
// Pass list: RecordCommandBuffer walks m_passes in order -- Compute passes record the dispatch list, Raster passes begin rendering on their
// target and record their draw lists. Default list: Compute (nothing while there are no dispatches) -> Shadow (skipped while it has nothing to
// draw) -> Main (Opaque, Transparent, overlay).
//
// Views: a window draws up to MAX_VIEWS cameras. Each view has its own draw lists (SetDraws) and its own frame table (pc.frame) naming its
// camera; a Raster pass records one view's lists into its area of the target (viewport and scissor). Several passes may target the swapchain:
// the first clears the whole image (render area = the image), the later ones load it and render inside their own area, the last ends in
// PRESENT_SRC -- so quad view is four view passes, then a whole-window pass for the overlay.
//
// Draws: a raster item needs a vertex buffer only when its pipeline's vertex shader reads inputs; otherwise it draws index_count vertices
// (or its indirect commands) with nothing bound -- procedural geometry from gl_VertexIndex and storage buffers.
//
// Dynamic rendering (Vulkan 1.3): passes are vkCmdBeginRendering on the swapchain / shadow images, with synchronization2 layout barriers around
// them. Cull mode, front face and depth test / write are dynamic -- set per draw from the pipeline's description and the draw item
// (RecordDynamicState), so a new pipeline description never needs a case here.
//
// Sync scoping:
//   m_semaphores_available, m_fences_in_flight, m_cmd_buffers -- one per frame in flight, indexed by m_current_frame
//   m_semaphores_finished                                     -- one per swapchain image, indexed by m_index, fresh for every swapchain
// =============================================================================================================================================
class Ir77PVCmdBuffer : public Ir77Enlisted, public IIr77PVCmdBuffer, public std::enable_shared_from_this<Ir77PVCmdBuffer> {
   public:
    Ir77PVCmdBuffer() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();

        Ir77PVPassDesc compute{};
        compute.type = Ir77PVPassType::Compute;

        Ir77PVPassDesc shadow{};
        shadow.target = Ir77PVPassTarget::Shadow;
        shadow.formats = Ir77PVRenderPassKind::Shadow;
        shadow.lists = {Ir77PVPass::Shadow};

        Ir77PVPassDesc main{};
        main.target = Ir77PVPassTarget::Swapchain;
        main.formats = Ir77PVRenderPassKind::Main;
        main.lists = {Ir77PVPass::Opaque, Ir77PVPass::Transparent};
        main.overlay = true;

        m_passes = {compute, shadow, main};
    }

    // The only class here that waits on its own -- it owns what is still in flight (command buffers, fences, present semaphores)
    ~Ir77PVCmdBuffer() {
        if (!m_device) return;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        for (auto const semaphore : m_semaphores_available)
            if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(device, semaphore, nullptr);

        for (auto const fence : m_fences_in_flight)
            if (fence != VK_NULL_HANDLE) vkDestroyFence(device, fence, nullptr);

        DestroySemaphores(device, m_semaphores_finished);
        DestroySemaphores(device, m_semaphores_retired);

        // frees m_cmd_buffers with it
        if (m_cmd_pool != VK_NULL_HANDLE) vkDestroyCommandPool(device, m_cmd_pool, nullptr);
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVCmdBuffer>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVCmdBuffer)
            obj = std::shared_ptr<IIr77PVCmdBuffer>(shared_from_this(), static_cast<IIr77PVCmdBuffer*>(this));

        else if (iid == GUIDIr77PVCmdBuffer)
            obj = std::shared_ptr<Ir77PVCmdBuffer>(shared_from_this(), static_cast<Ir77PVCmdBuffer*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // configuration
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) {
        m_swapchain = swapchain;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetRenderPass(Ir77PVRenderPassKind const& kind, std::shared_ptr<IIr77PVRenderPass> render_pass) {
        m_render_passes[kind] = render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Depth image the Shadow / ShadowSkinned pipelines render into (in the Shadow pass's depth format); the main pass samples it afterwards
    std::shared_ptr<IIr77Return const> SetShadowTarget(VkImage const& image, VkImageView const& view, VkExtent2D const& extent) {
        m_shadow_image = image;
        m_shadow_view = view;
        m_shadow_extent = extent;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The device's one pipeline layout -- push constants and the set bind go through it
    std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) {
        m_layout = layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The device's one bindless table
    std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) {
        m_descriptor_set = descriptor_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // This window's frame table slot per view + the slots they list, one entry per frame in flight
    std::shared_ptr<IIr77Return const> SetFrameSlots(std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT> const& frame_slots) {
        m_frame_slots = frame_slots;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Buffer slot of the device's material table (Ir77PeregrineV::GetMaterialTableSlot)
    std::shared_ptr<IIr77Return const> SetMaterialTable(std::uint32_t const& slot) {
        m_material_table = slot;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The device's pipeline cache -- draw items and the overlay carry pipeline handles, resolved through it while recording
    std::shared_ptr<IIr77Return const> SetPipelineCache(std::shared_ptr<IIr77PVPipelineCache> pipeline_cache) {
        m_pipeline_cache = pipeline_cache;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetOverlay(std::shared_ptr<IIr77PVOverlay> overlay, Ir77PVPipelineHandle const& pipeline) {
        m_overlay = overlay;
        m_overlay_pipeline = pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetClearColor(VkClearColorValue const& clear_color) {
        m_clear_color = clear_color;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // At least one Raster pass must target the swapchain -- the first clears it, the last ends in PRESENT_SRC. Every pass names a view below
    // MAX_VIEWS and an area inside its target.
    std::shared_ptr<IIr77Return const> SetPassList(std::vector<Ir77PVPassDesc> const& passes) {
        std::size_t swapchain_passes{0};

        for (auto const& pass : passes) {
            if (pass.type == Ir77PVPassType::Raster && pass.target == Ir77PVPassTarget::Swapchain) swapchain_passes++;

            for (auto const list : pass.lists)
                if (static_cast<std::size_t>(list) >= PASS_COUNT)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: pass names an unknown draw list.");

            if (pass.view >= MAX_VIEWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: pass names a view past MAX_VIEWS.");

            glm::vec4 const& area = pass.area;
            if (area.x < 0.0f || area.y < 0.0f || area.z <= 0.0f || area.w <= 0.0f || area.x + area.z > 1.0f || area.y + area.w > 1.0f)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: pass area must lie inside its target.");
        }

        if (swapchain_passes == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: the pass list needs a swapchain pass.");

        m_passes = passes;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetPassList(std::vector<Ir77PVPassDesc>& passes) {
        passes = m_passes;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The compute list, replaced whole -- recorded in order by every Compute pass. Empty = no compute work.
    std::shared_ptr<IIr77Return const> SetDispatches(std::vector<Ir77PVDispatch> const& dispatches) {
        m_dispatches = dispatches;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // The acceleration structures this command buffer builds, right after its Compute pass. Paint sets it every frame on the first window it
    // records and clears it on the rest, so the TLAS is built once a frame. Unset = no builds.
    std::shared_ptr<IIr77Return const> SetAcceleration(std::shared_ptr<IIr77PVAcceleration> acceleration) {
        m_acceleration = acceleration;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /*
    // -------------------------------------------------------------------------------------------------------------------------------------
    // ray tracing / acceleration structures
    // Needs: IIr77PVPipelineRT (owns the SBT), IIr77PVAccelStructure, and the device extensions
    //   VK_KHR_acceleration_structure, VK_KHR_ray_tracing_pipeline, VK_KHR_deferred_host_operations, VK_KHR_buffer_device_address.
    // Resolve m_trace_rays / m_build_accel in DefineExtensionFunctions.
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Ray-tracing pipeline (built on the shared layout) + what its shaders read -- TLAS / output image / instance table slots in push.user[]
    std::shared_ptr<IIr77Return const> SetRayTracing(std::shared_ptr<IIr77PVPipelineRT> pipeline, Ir77PVPushConstants const& push) {
        m_ray_pipeline = pipeline;
        m_ray_push = push;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Storage image the raygen shader writes; the main pass samples it afterwards. Must track swapchain size on resize.
    std::shared_ptr<IIr77Return const> SetRayTarget(VkImage const& image, VkExtent2D const& extent) {
        m_ray_target = image;
        m_ray_extent = extent;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Top-level structure rebuilt each frame from the instance list. Also used by ray queries in fragment / compute.
    std::shared_ptr<IIr77Return const> SetAccelStructure(std::shared_ptr<IIr77PVAccelStructure> tlas) {
        m_tlas = tlas;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }
    */

    // -------------------------------------------------------------------------------------------------------------------------------------
    // create
    // -------------------------------------------------------------------------------------------------------------------------------------
    // Pool, per-frame command buffers, per-frame sync objects and extension commands. Present semaphores follow the swapchain -- see
    // DefinePresentSemaphores.
    std::shared_ptr<IIr77Return const> CreateCommandBuffer() {
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: SetDevice not called.");
        if (m_cmd_pool != VK_NULL_HANDLE) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: command buffer already created.");

        auto result = DefineExtensionFunctions();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DispatchOperationPool();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DispatchOperationBuffers();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineSyncObjects();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Command Buffer.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // per frame
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> SetCurrentFrame(std::uint32_t const& current_frame) {
        if (current_frame >= MAX_FRAMES_IN_FLIGHT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: frame index out of range.");

        m_current_frame = current_frame;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Replaces one view's list for one pass. Opaque should be sorted by pipeline then material; Transparent back to front from that view's eye.
    std::shared_ptr<IIr77Return const> SetDraws(std::uint32_t const& view, Ir77PVPass const& pass, std::vector<Ir77PVDrawItem> const& draws) {
        std::size_t const list{static_cast<std::size_t>(pass)};
        if (view >= MAX_VIEWS || list >= PASS_COUNT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: unknown view or pass.");

        m_draws[view][list] = draws;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> ClearDraws() {
        for (auto& lists : m_draws)
            for (auto& list : lists) list.clear();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> WaitForFence() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        if (vkWaitForFences(device, 1, &m_fences_in_flight[m_current_frame], VK_TRUE, UINT64_MAX) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkWaitForFences failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Every frame-in-flight fence of this window, instead of vkDeviceWaitIdle
    std::shared_ptr<IIr77Return const> WaitAllFrames() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        if (vkWaitForFences(device, static_cast<std::uint32_t>(m_fences_in_flight.size()), m_fences_in_flight.data(), VK_TRUE, UINT64_MAX) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: WaitAllFrames failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // SUBOPTIMAL and OUT_OF_DATE are handed back in acquire_next_result for the caller to rebuild; anything else is an error
    std::shared_ptr<IIr77Return const> AcquireNextImage(VkResult* acquire_next_result) {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkSwapchainKHR swapchain{VK_NULL_HANDLE};
        m_swapchain->GetSwapchain(&swapchain);

        *acquire_next_result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, m_semaphores_available[m_current_frame], VK_NULL_HANDLE, &m_index);

        if (*acquire_next_result != VK_SUCCESS && *acquire_next_result != VK_SUBOPTIMAL_KHR && *acquire_next_result != VK_ERROR_OUT_OF_DATE_KHR)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkAcquireNextImageKHR failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Only after a successful acquire -- resetting then bailing on OUT_OF_DATE deadlocks the next WaitForFence
    std::shared_ptr<IIr77Return const> ResetFence() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        vkResetFences(device, 1, &m_fences_in_flight[m_current_frame]);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Walks the pass list in order. A failed pass ends the command buffer and returns its error.
    std::shared_ptr<IIr77Return const> RecordCommandBuffer() {
        if (m_cmd_buffers.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: CreateCommandBuffer not called.");

        if (m_render_passes.count(Ir77PVRenderPassKind::Main) == 0 || !m_swapchain)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: main render pass or swapchain not set.");

        if (!m_layout || !m_descriptor_set) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: SetLayout / SetDescriptorSet not called.");

        if (!m_pipeline_cache) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: SetPipelineCache not called.");

        if (m_frame_slots[m_current_frame].tables[0] == BINDLESS_NO_SLOT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: SetFrameSlots not called.");

        auto result = DefinePresentSemaphores();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        VkCommandBuffer const cmd{m_cmd_buffers[m_current_frame]};

        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkBeginCommandBuffer failed.");

        // one set for everything -- bound once here, untouched by pipeline switches
        m_descriptor_set->Bind(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);

        // the first swapchain pass clears the image, the last presents it
        std::size_t first{m_passes.size()};
        std::size_t last{m_passes.size()};

        for (std::size_t i = 0; i < m_passes.size(); i++) {
            if (m_passes[i].type != Ir77PVPassType::Raster || m_passes[i].target != Ir77PVPassTarget::Swapchain) continue;

            if (first == m_passes.size()) first = i;
            last = i;
        }

        // the Compute pass also builds the acceleration structures (when this command buffer has them) -- after the dispatches, so the
        // skinned vertices are posed before the BLAS refits read them
        for (std::size_t i = 0; i < m_passes.size(); i++) {
            Ir77PVPassDesc const& pass = m_passes[i];

            result = pass.type == Ir77PVPassType::Compute ? RecordCompute(cmd) : RecordRasterPass(cmd, pass, i == first, i == last);

            if (result->ID() == GUIDIr77OperationSucceeded && pass.type == Ir77PVPassType::Compute && m_acceleration)
                result = m_acceleration->RecordBuild(cmd);

            if (result->ID() != GUIDIr77OperationSucceeded) {
                vkEndCommandBuffer(cmd);
                return result;
            }
        }

        if (vkEndCommandBuffer(cmd) != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkEndCommandBuffer failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Waits on this frame's acquire semaphore, signals this image's finished semaphore and this frame's fence
    std::shared_ptr<IIr77Return const> SubmitFrame() {
        VkPipelineStageFlags const wait_stage{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &m_semaphores_available[m_current_frame];
        submit_info.pWaitDstStageMask = &wait_stage;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_cmd_buffers[m_current_frame];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &m_semaphores_finished[m_index];

        if (vkQueueSubmit(m_queue, 1, &submit_info, m_fences_in_flight[m_current_frame]) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkQueueSubmit failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // SUBOPTIMAL and OUT_OF_DATE are handed back in queue_present_result for the caller to rebuild
    std::shared_ptr<IIr77Return const> PresentFrame(VkResult* queue_present_result) {
        VkSwapchainKHR swapchain{VK_NULL_HANDLE};
        m_swapchain->GetSwapchain(&swapchain);

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &m_semaphores_finished[m_index];
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &m_index;

        *queue_present_result = vkQueuePresentKHR(m_queue, &present_info);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // queries
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> GetCurrentFrame(std::uint32_t* current_frame) {
        *current_frame = m_current_frame;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetImageIndex(std::uint32_t* image_index) {
        *image_index = m_index;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetCommandPool(VkCommandPool* pool) {
        *pool = m_cmd_pool;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetQueue(VkQueue* queue) {
        *queue = m_queue;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // steps
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Extension commands aren't exported by the loader; they stay null when the extension isn't enabled and each Record path checks its own
    std::shared_ptr<IIr77Return const> DefineExtensionFunctions() {
        bool mesh{false};
        m_device->GetMeshShading(mesh);

        if (!mesh) return Ir77RETURN<Ir77OperationSucceeded>();

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        m_draw_mesh_tasks = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT"));
        m_draw_mesh_tasks_indirect = reinterpret_cast<PFN_vkCmdDrawMeshTasksIndirectEXT>(vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectEXT"));

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // On the present family, so recording, submit and present all share one queue
    std::shared_ptr<IIr77Return const> DispatchOperationPool() {
        auto const result = m_device->GetPresentQueue(m_queue, m_queue_family);
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = m_queue_family;

        if (vkCreateCommandPool(device, &pool_info, nullptr, &m_cmd_pool) != VK_SUCCESS)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkCreateCommandPool failed.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DispatchOperationBuffers() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        m_cmd_buffers.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = m_cmd_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        if (vkAllocateCommandBuffers(device, &alloc_info, m_cmd_buffers.data()) != VK_SUCCESS) {
            m_cmd_buffers.clear();
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkAllocateCommandBuffers failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Acquire semaphores and frame fences, one per frame in flight. Fences start signalled so the first WaitForFence returns at once.
    std::shared_ptr<IIr77Return const> DefineSyncObjects() {
        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        m_semaphores_available.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
        m_fences_in_flight.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphore_info, nullptr, &m_semaphores_available[i]) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: acquire semaphore creation failed.");

            if (vkCreateFence(device, &fence_info, nullptr, &m_fences_in_flight[i]) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: frame fence creation failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One finished semaphore per swapchain image, made fresh for every swapchain (a resize keeps the image count but not the swapchain).
    // The presentation engine holds a finished semaphore until its image is re-acquired, which a retired swapchain never does -- so the
    // previous set is parked in m_semaphores_retired and destroyed one rebuild later, when nothing can still be waiting on it.
    // A no-op unless the swapchain changed; called from every RecordCommandBuffer.
    std::shared_ptr<IIr77Return const> DefinePresentSemaphores() {
        VkSwapchainKHR swapchain{VK_NULL_HANDLE};
        m_swapchain->GetSwapchain(&swapchain);

        std::vector<VkImage> images{};
        m_swapchain->GetSwapchainImages(images);

        std::size_t const image_count{images.size()};

        if (swapchain == m_present_swapchain && image_count == m_semaphores_finished.size()) return Ir77RETURN<Ir77OperationSucceeded>();

        if (image_count == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: swapchain has no images.");

        VkDevice device{VK_NULL_HANDLE};
        m_device->GetDevice(&device);

        DestroySemaphores(device, m_semaphores_retired);

        m_semaphores_retired = std::move(m_semaphores_finished);

        m_semaphores_finished.assign(image_count, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (std::size_t i = 0; i < image_count; i++)
            if (vkCreateSemaphore(device, &semaphore_info, nullptr, &m_semaphores_finished[i]) != VK_SUCCESS)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: present semaphore creation failed.");

        m_present_swapchain = swapchain;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // recording
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // What is currently bound / set, so consecutive draws sharing state skip the call. MAX_ENUM / UINT32_MAX = not set yet in this pass.
    struct Ir77PVBindState {
        VkPipeline pipeline{VK_NULL_HANDLE};
        VkBuffer vertex{VK_NULL_HANDLE};
        VkBuffer index{VK_NULL_HANDLE};
        VkIndexType index_type{VK_INDEX_TYPE_UINT32};
        VkCullModeFlags cull{VK_CULL_MODE_FLAG_BITS_MAX_ENUM};
        VkFrontFace front_face{VK_FRONT_FACE_MAX_ENUM};
        VkBool32 depth_test{UINT32_MAX};
        VkBool32 depth_write{UINT32_MAX};
        std::uint32_t shadow_tile{UINT32_MAX};  // atlas tile the viewport is on (Shadow pass)
    };

    // The images one Raster pass renders into, and the layouts they end in
    struct Ir77PVPassImages {
        VkImage color{VK_NULL_HANDLE};
        VkImageView color_view{VK_NULL_HANDLE};
        VkImage depth{VK_NULL_HANDLE};
        VkImageView depth_view{VK_NULL_HANDLE};
        VkImageAspectFlags depth_aspect{VK_IMAGE_ASPECT_DEPTH_BIT};
        VkExtent2D extent{};
        VkImageLayout color_final{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};          // PRESENT_SRC for the swapchain
        VkImageLayout depth_final{VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};  // READ_ONLY when sampled afterwards
    };

    // One Raster pass: -> attachment layouts, its view's draw lists in order inside its area, [overlay], -> final layouts.
    //   shadow pass:            contents discarded, cleared, the whole target, -> sampled
    //   first swapchain pass:   contents discarded, the whole image cleared (render area = the image), drawn inside its area
    //   later swapchain passes: loaded, render area = their own area -- the passes before them stay as they were
    //   last swapchain pass:    -> PRESENT_SRC; every other swapchain pass stores depth for the next one
    // Swapchain passes always record (the image has to be cleared and presented); any other pass is skipped while it has nothing to draw or
    // no target.
    std::shared_ptr<IIr77Return const> RecordRasterPass(VkCommandBuffer cmd, Ir77PVPassDesc const& pass, bool const first, bool const last) {
        bool const swapchain{pass.target == Ir77PVPassTarget::Swapchain};

        if (!swapchain && !HasDraws(pass)) return Ir77RETURN<Ir77OperationSucceeded>();

        Ir77PVPassImages images{};

        if (!ResolvePassImages(pass, images)) {
            if (swapchain) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: swapchain pass has no attachments for this image.");
            return Ir77RETURN<Ir77OperationSucceeded>();  // no target yet -- normal, not an error
        }

        bool const has_color{images.color_view != VK_NULL_HANDLE};
        bool const has_depth{images.depth_view != VK_NULL_HANDLE};
        bool const sampled_depth{images.depth_final == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

        bool const fresh{!swapchain || first};  // starts from nothing -- discarded and cleared
        bool const ends{!swapchain || last};    // leaves the attachments in their final layouts

        VkRect2D const area{PassArea(pass.area, images.extent)};
        VkRect2D const whole{{0, 0}, images.extent};

        Barrier(cmd, fresh ? FreshBarriers(images) : LoadBarriers(images));

        VkRenderingAttachmentInfo color{};
        color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color.imageView = images.color_view;
        color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.loadOp = fresh ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.clearValue.color = m_clear_color;

        // reversed-Z everywhere but the Shadow pass: the camera maps near to 1 and far to 0 (Ir77View), so depth clears to 0 and the greater
        // depth wins -- float depth is densest near 0, where the distance is, so precision is near-uniform across the view. The Shadow pass
        // keeps standard depth: its matrices are Lighting's, and its depth bias and compare sampler expect it.
        bool const reversed{pass.target != Ir77PVPassTarget::Shadow};

        VkRenderingAttachmentInfo depth{};
        depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depth.imageView = images.depth_view;
        depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth.loadOp = fresh ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        depth.storeOp = sampled_depth || !ends ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.clearValue.depthStencil = {reversed ? 0.0f : 1.0f, 0};

        VkRenderingInfo rendering_info{};
        rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering_info.renderArea = fresh ? whole : area;
        rendering_info.layerCount = 1;
        rendering_info.colorAttachmentCount = has_color ? 1 : 0;
        rendering_info.pColorAttachments = has_color ? &color : nullptr;
        rendering_info.pDepthAttachment = has_depth ? &depth : nullptr;

        vkCmdBeginRendering(cmd, &rendering_info);

        SetPassState(cmd, area, reversed);

        for (auto const list : pass.lists) RecordDraws(cmd, pass.view, m_draws[pass.view][static_cast<std::size_t>(list)]);

        if (pass.overlay) RecordOverlay(cmd, pass.view);

        vkCmdEndRendering(cmd);

        if (!ends) return Ir77RETURN<Ir77OperationSucceeded>();

        // -> presentation (the finished semaphore SubmitFrame signals orders the present after this) / -> sampled by later passes
        std::vector<VkImageMemoryBarrier2> after{};

        if (has_color && images.color_final != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            after.push_back(ImageBarrier(images.color, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, images.color_final,
                                         VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_NONE,
                                         VK_ACCESS_2_NONE));

        if (has_depth && sampled_depth)
            after.push_back(ImageBarrier(images.depth, images.depth_aspect, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, images.depth_final,
                                         VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                         VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT));

        if (!after.empty()) Barrier(cmd, after);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // A pass that starts from nothing: contents discarded -> attachment layouts. Color after the acquire semaphore (waited at
    // COLOR_ATTACHMENT_OUTPUT); depth after last frame's writes / sampling.
    std::vector<VkImageMemoryBarrier2> FreshBarriers(Ir77PVPassImages const& images) const {
        std::vector<VkImageMemoryBarrier2> barriers{};

        if (images.color_view != VK_NULL_HANDLE)
            barriers.push_back(ImageBarrier(images.color, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT));

        if (images.depth_view != VK_NULL_HANDLE)
            barriers.push_back(ImageBarrier(
                images.depth, images.depth_aspect, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT));

        return barriers;
    }

    // A later swapchain pass: the attachments stay in their layouts; the previous pass's writes finish before this one loads and writes
    std::vector<VkImageMemoryBarrier2> LoadBarriers(Ir77PVPassImages const& images) const {
        std::vector<VkImageMemoryBarrier2> barriers{};

        if (images.color_view != VK_NULL_HANDLE)
            barriers.push_back(ImageBarrier(images.color, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                                            VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT));

        if (images.depth_view != VK_NULL_HANDLE)
            barriers.push_back(ImageBarrier(images.depth, images.depth_aspect, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                                            VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT));

        return barriers;
    }

    // Outside any render pass: the dispatch list in order. After each one a barrier makes its writes visible to the next dispatch and to every
    // draw -- indirect commands, index / vertex data, storage buffers and images in any shader stage.
    std::shared_ptr<IIr77Return const> RecordCompute(VkCommandBuffer cmd) {
        if (m_dispatches.empty()) return Ir77RETURN<Ir77OperationSucceeded>();  // no compute work -- not an error

        m_descriptor_set->Bind(cmd, VK_PIPELINE_BIND_POINT_COMPUTE);

        // storage buffers are shared by the frames in flight: the previous frame's draws must be done reading before this frame writes
        VkMemoryBarrier2 previous{};
        previous.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        previous.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        previous.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;

        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.memoryBarrierCount = 1;
        dependency.pMemoryBarriers = &previous;

        vkCmdPipelineBarrier2(cmd, &dependency);

        VkPipeline bound{VK_NULL_HANDLE};

        for (auto const& dispatch : m_dispatches) {
            std::shared_ptr<IIr77PVPipelineCPT> resolved{};
            if (m_pipeline_cache->GetComputePipeline(dispatch.pipeline, resolved)->ID() != GUIDIr77OperationSucceeded) continue;

            VkPipeline pipeline{VK_NULL_HANDLE};
            resolved->GetPipeline(&pipeline);

            if (pipeline == VK_NULL_HANDLE) continue;

            if (pipeline != bound) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                bound = pipeline;
            }

            Ir77PVPushConstants push{dispatch.push};
            Ir77PVPushConstants const base{BasePush(0)};

            push.frame = base.frame;
            push.instances = base.instances;
            push.bones = base.bones;
            push.materials = base.materials;

            Push(cmd, push);

            if (dispatch.indirect) {
                VkBuffer indirect{VK_NULL_HANDLE};
                dispatch.indirect->GetBuffer(m_current_frame, &indirect);

                vkCmdDispatchIndirect(cmd, indirect, dispatch.indirect_offset);
            } else {
                vkCmdDispatch(cmd, dispatch.groups[0], dispatch.groups[1], dispatch.groups[2]);
            }

            RecordComputeBarrier(cmd);
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // compute writes -> the next dispatch and every draw stage that can read them
    void RecordComputeBarrier(VkCommandBuffer cmd) const {
        VkPipelineStageFlags2 dst_stages{VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT |
                                         VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT};

        if (m_draw_mesh_tasks) dst_stages |= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT | VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT;

        VkMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
        barrier.dstStageMask = dst_stages;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT |
                                VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT;

        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.memoryBarrierCount = 1;
        dependency.pMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(cmd, &dependency);
    }

    /*
    // Rebuilds the TLAS from this frame's instance transforms (BLASes are built once at load by Asset).
    // Outside any render pass. The barrier makes the build visible to ray tracing and to ray queries in fragment / compute.
    std::shared_ptr<IIr77Return const> RecordAccelBuild(VkCommandBuffer cmd) {
        if (!m_tlas || !m_build_accel) return Ir77RETURN<Ir77OperationSucceeded>();

        VkAccelerationStructureBuildGeometryInfoKHR build_info{};
        VkAccelerationStructureBuildRangeInfoKHR const* range{nullptr};

        if (m_tlas->GetBuildInfo(m_current_frame, &build_info, &range)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: TLAS build info unavailable.");

        m_build_accel(cmd, 1, &build_info, &range);

        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                             VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
                             &barrier, 0, nullptr, 0, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Raygen writes m_ray_target; the main pass samples it. Outside any render pass.
    std::shared_ptr<IIr77Return const> RecordRayTrace(VkCommandBuffer cmd) {
        if (!m_ray_pipeline || !m_trace_rays || m_ray_target == VK_NULL_HANDLE) return Ir77RETURN<Ir77OperationSucceeded>();

        VkPipeline pipeline{VK_NULL_HANDLE};
        m_ray_pipeline->GetPipeline(&pipeline);

        VkStridedDeviceAddressRegionKHR raygen{};
        VkStridedDeviceAddressRegionKHR miss{};
        VkStridedDeviceAddressRegionKHR hit{};
        VkStridedDeviceAddressRegionKHR callable{};
        m_ray_pipeline->GetShaderBindingTable(&raygen, &miss, &hit, &callable);

        // last frame's sampled image -> writable storage image (contents discarded, raygen overwrites every texel)
        VkImageMemoryBarrier to_write{};
        to_write.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        to_write.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        to_write.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        to_write.srcAccessMask = 0;
        to_write.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        to_write.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_write.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_write.image = m_ray_target;
        to_write.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0, nullptr, 0, nullptr, 1, &to_write);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);

        m_descriptor_set->Bind(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);

        Push(cmd, m_ray_push);  // TLAS / output image / instance table slots in user[]

        m_trace_rays(cmd, &raygen, &miss, &hit, &callable, m_ray_extent.width, m_ray_extent.height, 1);

        // storage image -> sampled by the main pass fragment shader
        VkImageMemoryBarrier to_read = to_write;
        to_read.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        to_read.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_read.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        to_read.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_read);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }
    */

    // Pipeline and vertex / index binds are skipped when unchanged from the previous item; the set never changes.
    // Mesh items read geometry from buffer slots and dispatch task groups; everything else binds vertex / index and draws.
    void RecordDraws(VkCommandBuffer cmd, std::uint32_t const view, std::vector<Ir77PVDrawItem> const& draws) {
        Ir77PVBindState state{};

        for (auto const& item : draws) {
            std::shared_ptr<IIr77PVPipeline> resolved{};
            if (m_pipeline_cache->GetPipeline(item.pipeline, resolved)->ID() != GUIDIr77OperationSucceeded) continue;

            VkPipeline pipeline{VK_NULL_HANDLE};
            resolved->GetPipeline(&pipeline);

            if (pipeline == VK_NULL_HANDLE) continue;

            Ir77PVPipelineDesc desc{};
            resolved->GetDesc(&desc);

            bool const mesh{desc.IsMesh()};

            bool vertex_input{false};
            resolved->GetVertexInput(vertex_input);

            // mesh items need the extension loaded and their meshlet slots; raster items need a vertex buffer when the shader reads one
            if (mesh ? (!m_draw_mesh_tasks || item.meshlet_slots[0] == BINDLESS_NO_SLOT) : (vertex_input && !item.vertex)) continue;

#ifndef NDEBUG
            // the slots the item brings must hold what their push-constant fields read -- otherwise the shader reads garbage
            if (!CheckItemSlots(mesh, item)) continue;
#endif

            if (pipeline != state.pipeline) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                state.pipeline = pipeline;
            }

            RecordDynamicState(cmd, desc, item, state);

            // shadow draws -- each light renders into its own atlas tile
            if (desc.pass == Ir77PVRenderPassKind::Shadow && item.light_index != state.shadow_tile) {
                std::uint32_t const tile_w = m_shadow_extent.width / SHADOW_ATLAS_SIDE;
                std::uint32_t const tile_h = m_shadow_extent.height / SHADOW_ATLAS_SIDE;
                std::uint32_t const tx = (item.light_index % SHADOW_ATLAS_SIDE) * tile_w;
                std::uint32_t const ty = (item.light_index / SHADOW_ATLAS_SIDE) * tile_h;

                VkViewport viewport{static_cast<float>(tx), static_cast<float>(ty), static_cast<float>(tile_w), static_cast<float>(tile_h), 0.0f, 1.0f};
                VkRect2D scissor{{static_cast<std::int32_t>(tx), static_cast<std::int32_t>(ty)}, {tile_w, tile_h}};

                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                state.shadow_tile = item.light_index;
            }

            Push(cmd, ItemPush(desc, item, view));

            if (mesh)
                RecordMeshDraw(cmd, item);
            else
                RecordRasterDraw(cmd, item, state);
        }
    }

    // Task groups over the item's meshlets -- direct, or from its indirect buffer
    void RecordMeshDraw(VkCommandBuffer cmd, Ir77PVDrawItem const& item) {
        if (item.indirect) {
            VkBuffer indirect{VK_NULL_HANDLE};
            item.indirect->GetBuffer(m_current_frame, &indirect);

            m_draw_mesh_tasks_indirect(cmd, indirect, item.indirect_offset, item.indirect_count, sizeof(VkDrawMeshTasksIndirectCommandEXT));
        } else {
            m_draw_mesh_tasks(cmd, (item.meshlet_count + TASK_GROUP_SIZE - 1) / TASK_GROUP_SIZE, 1, 1);
        }
    }

    // Vertex / optional index binding, then direct or indirect draw. Without an index buffer, index_count is the vertex count.
    // No vertex buffer = procedural: nothing bound, the shader builds its vertices
    void RecordRasterDraw(VkCommandBuffer cmd, Ir77PVDrawItem const& item, Ir77PVBindState& state) {
        if (item.vertex) {
            VkBuffer vertex{VK_NULL_HANDLE};
            item.vertex->GetBuffer(m_current_frame, &vertex);

            if (vertex != state.vertex) {
                VkDeviceSize const offset{0};
                vkCmdBindVertexBuffers(cmd, 0, 1, &vertex, &offset);
                state.vertex = vertex;
            }
        }

        bool const indexed{static_cast<bool>(item.index)};

        if (indexed) {
            VkBuffer index{VK_NULL_HANDLE};
            item.index->GetBuffer(m_current_frame, &index);

            if (index != state.index || item.index_type != state.index_type) {
                vkCmdBindIndexBuffer(cmd, index, 0, item.index_type);
                state.index = index;
                state.index_type = item.index_type;
            }
        }

        if (item.indirect) {
            VkBuffer indirect{VK_NULL_HANDLE};
            item.indirect->GetBuffer(m_current_frame, &indirect);

            if (indexed)
                vkCmdDrawIndexedIndirect(cmd, indirect, item.indirect_offset, item.indirect_count, sizeof(VkDrawIndexedIndirectCommand));
            else
                vkCmdDrawIndirect(cmd, indirect, item.indirect_offset, item.indirect_count, sizeof(VkDrawIndirectCommand));
        } else if (indexed) {
            vkCmdDrawIndexed(cmd, item.index_count, item.instance_count, item.first_index, item.vertex_offset, item.first_instance);
        } else {
            vkCmdDraw(cmd, item.index_count, item.instance_count, 0, item.first_instance);
        }
    }

    // Browser composite, drawn last over the scene. Skipped while the overlay is resizing or before its first upload.
    // The CEF shaders read the overlay texture slot from pc.user[0]
    void RecordOverlay(VkCommandBuffer cmd, std::uint32_t const view) {
        if (!m_overlay || m_overlay_pipeline == IR77PV_NO_PIPELINE) return;

        std::shared_ptr<IIr77PVPipeline> resolved{};
        if (m_pipeline_cache->GetPipeline(m_overlay_pipeline, resolved)->ID() != GUIDIr77OperationSucceeded) return;

        bool resizing{false};
        m_overlay->IsResizing(resizing);

        bool ever_uploaded{false};
        m_overlay->HasEverUploaded(ever_uploaded);

        if (resizing || !ever_uploaded) return;

        VkPipeline pipeline{VK_NULL_HANDLE};
        resolved->GetPipeline(&pipeline);

        Ir77PVPushConstants push{BasePush(view)};
        m_overlay->GetTextureSlot(push.user[0]);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        Ir77PVPipelineDesc desc{};
        resolved->GetDesc(&desc);

        Ir77PVBindState state{};
        RecordDynamicState(cmd, desc, Ir77PVDrawItem{}, state);

        Push(cmd, push);

        vkCmdDraw(cmd, OVERLAY_VERTICES, 1, 0, 0);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // internals
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    // Any of the pass's draw lists (its view's) non-empty, or it composites the overlay
    bool HasDraws(Ir77PVPassDesc const& pass) const {
        if (pass.overlay) return true;

        for (auto const list : pass.lists)
            if (!m_draws[pass.view][static_cast<std::size_t>(list)].empty()) return true;

        return false;
    }

    // Fills the images a Raster pass renders into for this frame; false when its target doesn't exist (yet)
    bool ResolvePassImages(Ir77PVPassDesc const& pass, Ir77PVPassImages& images) const {
        auto const formats = GetPass(pass.formats);
        if (!formats) return false;

        VkFormat depth_format{VK_FORMAT_UNDEFINED};
        formats->GetDepthFormat(&depth_format);

        images.depth_aspect = DepthAspect(depth_format);

        switch (pass.target) {
            case Ir77PVPassTarget::Swapchain: {
                std::vector<VkImage> swapchain_images{};
                m_swapchain->GetSwapchainImages(swapchain_images);

                std::vector<VkImageView> views{};
                m_swapchain->GetSwapchainViews(views);

                if (m_index >= swapchain_images.size() || m_index >= views.size()) return false;

                images.color = swapchain_images[m_index];
                images.color_view = views[m_index];
                images.color_final = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                if (depth_format != VK_FORMAT_UNDEFINED) {
                    m_swapchain->GetDepthTarget(images.depth, images.depth_view);
                    if (images.depth_view == VK_NULL_HANDLE) return false;
                }

                m_swapchain->GetSwapchainExtents(images.extent);

                return true;
            }

            case Ir77PVPassTarget::Shadow:
                if (m_shadow_view == VK_NULL_HANDLE) return false;

                images.depth = m_shadow_image;
                images.depth_view = m_shadow_view;
                images.depth_final = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                images.extent = m_shadow_extent;

                return true;

            default:
                return false;
        }
    }

    // The pass's format description; empty when that kind wasn't set
    std::shared_ptr<IIr77PVRenderPass> GetPass(Ir77PVRenderPassKind const& kind) const {
        auto const found = m_render_passes.find(kind);
        if (found == m_render_passes.end()) return std::shared_ptr<IIr77PVRenderPass>{};

        return found->second;
    }

    // Per-frame fields every push shares: the view's frame table, instances, frame bones, material table
    Ir77PVPushConstants BasePush(std::uint32_t const view) const {
        Ir77PVFrameSlots const& frame = m_frame_slots[m_current_frame];

        Ir77PVPushConstants push{};
        push.frame = frame.tables[view];
        push.instances = frame.values.instances;
        push.bones = frame.values.bones;
        push.materials = m_material_table;

        return push;
    }

    // BasePush + the item. Mesh: pc.vertices = vertices slot, pc.user[0..3] = meshlets, meshlet vertices, meshlet triangles, draw records.
    // Shadow: pc.user[0] = light index. Every item: pc.user[4..7] = item.user (the manifest's "user" names).
    Ir77PVPushConstants ItemPush(Ir77PVPipelineDesc const& desc, Ir77PVDrawItem const& item, std::uint32_t const view) const {
        Ir77PVPushConstants push{BasePush(view)};

        push.model = item.model;
        push.material = item.material;
        push.mode = item.mode;
        push.draw_base = item.draw_base;

        if (item.bones != BINDLESS_NO_SLOT) push.bones = item.bones;

        if (desc.IsMesh()) {
            push.vertices = item.meshlet_slots[3];
            push.user[0] = item.meshlet_slots[0];
            push.user[1] = item.meshlet_slots[1];
            push.user[2] = item.meshlet_slots[2];
            push.user[3] = item.meshlet_slots[4];
        }

        if (desc.pass == Ir77PVRenderPassKind::Shadow) push.user[0] = item.light_index;

        for (std::size_t i = 0; i < item.user.size(); i++) push.user[USER_FIRST + i] = item.user[i];

        return push;
    }

    // Debug builds: every slot a draw item brings is checked against the kind its push-constant field reads -- bones -> Bones; mesh items'
    // meshlets / meshlet vertices / meshlet triangles / vertices / draw records. A mismatch (or a released slot: None) skips the draw and is
    // logged once per slot. The frame's own slots (frame table, instances, material table) are Init's and always right; user[] is the
    // pipeline's business and isn't checked.
    bool CheckItemSlots(bool const& mesh, Ir77PVDrawItem const& item) {
        auto const check = [&](char const* field, std::uint32_t const slot, Ir77PVSlotKind const expected) {
            if (slot == BINDLESS_NO_SLOT) return true;

            Ir77PVSlotKind kind{Ir77PVSlotKind::None};
            m_descriptor_set->GetSlotKind(BINDLESS_BINDING_BUFFERS, slot, kind);
            if (kind == expected) return true;

            if (m_slot_errors.insert(slot).second)
                Ir77RETURN<Ir77NotConfigured>(this, std::string{"Ir77PVCmdBuffer: draw skipped -- "} + field + " is buffer slot " + std::to_string(slot) +
                                                        ", which holds " + SLOT_KIND_NAMES[static_cast<std::size_t>(kind)] + ", not " +
                                                        SLOT_KIND_NAMES[static_cast<std::size_t>(expected)] + ".");
            return false;
        };

        bool ok = check("pc.bones", item.bones, Ir77PVSlotKind::Bones);

        if (mesh) {
            ok = check("pc.user[0] (meshlets)", item.meshlet_slots[0], Ir77PVSlotKind::Meshlets) && ok;
            ok = check("pc.user[1] (meshlet vertices)", item.meshlet_slots[1], Ir77PVSlotKind::MeshletVertices) && ok;
            ok = check("pc.user[2] (meshlet triangles)", item.meshlet_slots[2], Ir77PVSlotKind::MeshletTriangles) && ok;
            ok = check("pc.vertices", item.meshlet_slots[3], Ir77PVSlotKind::Vertices) && ok;
            ok = check("pc.user[3] (draw records)", item.meshlet_slots[4], Ir77PVSlotKind::MeshDraws) && ok;
        }

        return ok;
    }

    // One range, every stage -- the whole block each time
    void Push(VkCommandBuffer cmd, Ir77PVPushConstants const& push) const {
        VkPipelineLayout layout{VK_NULL_HANDLE};
        m_layout->GetPipelineLayout(&layout);

        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_ALL, 0, sizeof(Ir77PVPushConstants), &push);
    }

    // Once per pass, after vkCmdBeginRendering: viewport / scissor to the pass's area, and the pass's depth compare -- GREATER for reversed-Z
    // (every pass but Shadow), LESS for standard depth
    void SetPassState(VkCommandBuffer cmd, VkRect2D const& area, bool const reversed) const {
        VkViewport viewport{};
        viewport.x = static_cast<float>(area.offset.x);
        viewport.y = static_cast<float>(area.offset.y);
        viewport.width = static_cast<float>(area.extent.width);
        viewport.height = static_cast<float>(area.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        vkCmdSetScissor(cmd, 0, 1, &area);

        vkCmdSetDepthCompareOp(cmd, reversed ? VK_COMPARE_OP_GREATER : VK_COMPARE_OP_LESS);
    }

    // A pass's area (fractions of the target) in pixels. Both edges are rounded the same way, so areas that share an edge meet without a gap or
    // an overlap; never empty, never outside the target.
    VkRect2D PassArea(glm::vec4 const& area, VkExtent2D const& extent) const {
        std::uint32_t const width{std::max(extent.width, 1u)};
        std::uint32_t const height{std::max(extent.height, 1u)};

        auto const edge = [](float const fraction, std::uint32_t const size) {
            return static_cast<std::uint32_t>(std::lround(std::clamp(fraction, 0.0f, 1.0f) * static_cast<float>(size)));
        };

        std::uint32_t const x0{std::min(edge(area.x, width), width - 1)};
        std::uint32_t const y0{std::min(edge(area.y, height), height - 1)};
        std::uint32_t const x1{std::max(edge(area.x + area.z, width), x0 + 1)};
        std::uint32_t const y1{std::max(edge(area.y + area.w, height), y0 + 1)};

        return VkRect2D{{static_cast<std::int32_t>(x0), static_cast<std::int32_t>(y0)}, {x1 - x0, y1 - y0}};
    }

    // Per-draw raster / depth state the pipelines leave dynamic -- set only when it differs from the previous draw
    //   cull:  the description's, except back_faces culls front faces (a transparent object's far side) and double_sided turns an opaque
    //          description's back-face culling off
    //   front: clockwise for mirrored instances (negative determinant), counter-clockwise otherwise
    //   depth: the description's test / write
    void RecordDynamicState(VkCommandBuffer cmd, Ir77PVPipelineDesc const& desc, Ir77PVDrawItem const& item, Ir77PVBindState& state) const {
        VkCullModeFlags cull{desc.cull};

        if (item.back_faces)
            cull = VK_CULL_MODE_FRONT_BIT;
        else if (item.double_sided && desc.blend == Ir77PVBlend::Opaque && desc.cull == VK_CULL_MODE_BACK_BIT)
            cull = VK_CULL_MODE_NONE;

        VkFrontFace const front_face{item.mirrored ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE};
        VkBool32 const depth_test{desc.depth_test ? VK_TRUE : VK_FALSE};
        VkBool32 const depth_write{desc.depth_write ? VK_TRUE : VK_FALSE};

        if (cull != state.cull) {
            vkCmdSetCullMode(cmd, cull);
            state.cull = cull;
        }

        if (front_face != state.front_face) {
            vkCmdSetFrontFace(cmd, front_face);
            state.front_face = front_face;
        }

        if (depth_test != state.depth_test) {
            vkCmdSetDepthTestEnable(cmd, depth_test);
            state.depth_test = depth_test;
        }

        if (depth_write != state.depth_write) {
            vkCmdSetDepthWriteEnable(cmd, depth_write);
            state.depth_write = depth_write;
        }
    }

    VkImageMemoryBarrier2 ImageBarrier(VkImage const& image, VkImageAspectFlags const& aspect, VkImageLayout const& old_layout, VkImageLayout const& new_layout,
                                       VkPipelineStageFlags2 const& src_stage, VkAccessFlags2 const& src_access, VkPipelineStageFlags2 const& dst_stage,
                                       VkAccessFlags2 const& dst_access) const {
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = src_stage;
        barrier.srcAccessMask = src_access;
        barrier.dstStageMask = dst_stage;
        barrier.dstAccessMask = dst_access;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {aspect, 0, 1, 0, 1};

        return barrier;
    }

    void Barrier(VkCommandBuffer cmd, std::vector<VkImageMemoryBarrier2> const& barriers) const {
        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.imageMemoryBarrierCount = static_cast<std::uint32_t>(barriers.size());
        dependency.pImageMemoryBarriers = barriers.data();

        vkCmdPipelineBarrier2(cmd, &dependency);
    }

    // Combined depth / stencil formats transition both aspects together
    VkImageAspectFlags DepthAspect(VkFormat const& format) const {
        bool const stencil{format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT};

        return stencil ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    void DestroySemaphores(VkDevice device, std::vector<VkSemaphore>& semaphores) {
        for (auto const semaphore : semaphores)
            if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(device, semaphore, nullptr);

        semaphores.clear();
    }

   private:
    // must match local_size_x in the task shader
    static constexpr std::uint32_t TASK_GROUP_SIZE = 32;

    // overlay quad -- two triangles from gl_VertexIndex
    static constexpr std::uint32_t OVERLAY_VERTICES = 6;

    // one draw list per Ir77PVPass: Shadow, Opaque, Transparent
    static constexpr std::size_t PASS_COUNT = 3;

    // pc.user[4..7] -- item.user; user[0..3] are the engine's per-kind data
    static constexpr std::size_t USER_FIRST = 4;

    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVSwapchain> m_swapchain{};

    std::map<Ir77PVRenderPassKind, std::shared_ptr<IIr77PVRenderPass>> m_render_passes{};

    VkImage m_shadow_image{VK_NULL_HANDLE};

    VkImageView m_shadow_view{VK_NULL_HANDLE};

    VkExtent2D m_shadow_extent{};

    // bindless -- the device's layout and table, this window's frame slots, the material table slot
    std::shared_ptr<IIr77PVLayout> m_layout{};

    std::shared_ptr<IIr77PVDescriptorSet> m_descriptor_set{};

    std::set<std::uint32_t> m_slot_errors{};  // CheckItemSlots -- buffer slots already reported, so each is logged once

    std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT> m_frame_slots{};

    std::uint32_t m_material_table{BINDLESS_NO_SLOT};

    std::shared_ptr<IIr77PVOverlay> m_overlay{};

    Ir77PVPipelineHandle m_overlay_pipeline{IR77PV_NO_PIPELINE};

    // resolves draw items' and the overlay's pipeline handles
    std::shared_ptr<IIr77PVPipelineCache> m_pipeline_cache{};

    VkClearColorValue m_clear_color{{0.0f, 0.0f, 0.0f, 1.0f}};

    std::array<std::array<std::vector<Ir77PVDrawItem>, PASS_COUNT>, MAX_VIEWS> m_draws{};  // [view][list]

    // what RecordCommandBuffer records, in order -- set in the constructor, replaced by SetPassList
    std::vector<Ir77PVPassDesc> m_passes{};

    // compute
    // the compute list -- SetDispatches
    std::vector<Ir77PVDispatch> m_dispatches{};

    // acceleration structures -- SetAcceleration (the frame's first recorded window); built after the Compute pass
    std::shared_ptr<IIr77PVAcceleration> m_acceleration{};

    // queue and recording
    VkQueue m_queue{VK_NULL_HANDLE};

    std::uint32_t m_queue_family{0};

    VkCommandPool m_cmd_pool{VK_NULL_HANDLE};

    std::vector<VkCommandBuffer> m_cmd_buffers{};  // one per frame in flight

    std::uint32_t m_current_frame{0};

    std::uint32_t m_index{0};  // swapchain image from the last acquire

    // sync -- see the scoping note at the top of the class
    std::vector<VkSemaphore> m_semaphores_available{};

    std::vector<VkFence> m_fences_in_flight{};

    std::vector<VkSemaphore> m_semaphores_finished{};

    std::vector<VkSemaphore> m_semaphores_retired{};  // the previous swapchain's finished set, destroyed on the next rebuild

    VkSwapchainKHR m_present_swapchain{VK_NULL_HANDLE};  // swapchain m_semaphores_finished was made for

    // extension commands (DefineExtensionFunctions) -- null when the extension isn't enabled
    PFN_vkCmdDrawMeshTasksEXT m_draw_mesh_tasks{nullptr};

    PFN_vkCmdDrawMeshTasksIndirectEXT m_draw_mesh_tasks_indirect{nullptr};

    /*
    // ray tracing
    std::shared_ptr<IIr77PVPipelineRT> m_ray_pipeline{};

    Ir77PVPushConstants m_ray_push{};

    VkImage m_ray_target{VK_NULL_HANDLE};

    VkExtent2D m_ray_extent{};

    // TLAS here, BLASes owned by Asset
    std::shared_ptr<IIr77PVAccelStructure> m_tlas{};

    PFN_vkCmdTraceRaysKHR m_trace_rays{nullptr};

    PFN_vkCmdBuildAccelerationStructuresKHR m_build_accel{nullptr};
    */
};

}  // namespace NSIr77PeregrineV