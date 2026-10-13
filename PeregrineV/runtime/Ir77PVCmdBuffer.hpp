#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVCmdBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVCmdBuffer -- one window's frame loop: command pool and buffers, sync objects, recording, submit and present.
//
//   configure: SetDevice / SetSwapchain / SetRenderPass / SetShadowTarget / SetGlobalSet / SetPassSet / SetOverlay / SetClearColor / SetCompute*
//   create:    CreateCommandBuffer -> DefineExtensionFunctions -> DefineCommandPool -> DefineCommandBuffers -> DefineSyncObjects
//   per frame: SetCurrentFrame -> WaitForFence -> AcquireNextImage -> ResetFence -> RecordCommandBuffer -> SubmitFrame -> PresentFrame
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

    // Depth-only pass the Shadow / ShadowSkinned pipelines were built against, plus its shadow-map framebuffer
    std::shared_ptr<IIr77Return const> SetShadowTarget(VkFramebuffer const& framebuffer, VkExtent2D const& extent) {
        m_shadow_framebuffer = framebuffer;
        m_shadow_extent = extent;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Set 0 -- camera, lights, instances, env map. Bound once per pipeline-layout change.
    std::shared_ptr<IIr77Return const> SetGlobalSet(std::shared_ptr<IIr77PVDescriptorSet> global_set) {
        m_global_set = global_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Set 1 -- shadow map + shadow matrices. Main-pass Static / Skinned / Mesh only.
    std::shared_ptr<IIr77Return const> SetPassSet(std::shared_ptr<IIr77PVDescriptorSet> pass_set) {
        m_pass_set = pass_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetOverlay(std::shared_ptr<IIr77PVOverlay> overlay, std::shared_ptr<IIr77PVPipeline> pipeline,
                                                  std::shared_ptr<IIr77PVLayout> layout) {
        m_overlay = overlay;
        m_overlay_pipeline = pipeline;
        m_overlay_layout = layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetClearColor(VkClearColorValue const& clear_color) {
        m_clear_color = clear_color;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetComputePipeline(std::shared_ptr<IIr77PVPipelineCPT> pipeline) {
        m_compute_pipeline = pipeline;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetComputeLayout(std::shared_ptr<IIr77PVLayoutCPT> layout) {
        m_compute_layout = layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetComputeSets(std::vector<std::shared_ptr<IIr77PVDescriptorSet>> const& sets) {
        m_compute_sets = sets;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetComputeDispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
        m_compute_groups_x = x;
        m_compute_groups_y = y;
        m_compute_groups_z = z;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    /*
    // -------------------------------------------------------------------------------------------------------------------------------------
    // ray tracing / acceleration structures
    // Needs: IIr77PVPipelineRT (owns the SBT), IIr77PVLayoutRT, IIr77PVAccelStructure, and the device extensions
    //   VK_KHR_acceleration_structure, VK_KHR_ray_tracing_pipeline, VK_KHR_deferred_host_operations, VK_KHR_buffer_device_address.
    // Resolve m_trace_rays / m_build_accel in DefineExtensionFunctions.
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Ray-tracing pipeline + layout + its sets (set 0 usually: TLAS, output image, camera)
    std::shared_ptr<IIr77Return const> SetRayTracing(std::shared_ptr<IIr77PVPipelineRT> pipeline, std::shared_ptr<IIr77PVLayoutRT> layout,
                                                     std::vector<std::shared_ptr<IIr77PVDescriptorSet>> const& sets) {
        m_ray_pipeline = pipeline;
        m_ray_layout = layout;
        m_ray_sets = sets;

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

        result = DefineCommandPool();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        result = DefineCommandBuffers();
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

    // Replaces the list for one pass. Opaque should be sorted by pipeline then material; Transparent back to front.
    std::shared_ptr<IIr77Return const> SetDraws(Ir77PVPass const& pass, std::vector<Ir77PVDrawItem> const& draws) {
        std::size_t const slot{static_cast<std::size_t>(pass)};
        if (slot >= m_draws.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: unknown pass.");

        m_draws[slot] = draws;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> ClearDraws() {
        for (auto& list : m_draws) list.clear();

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

    // Order: compute -> TLAS build -> ray trace -> shadow pass -> main pass. Everything before the shadow pass records outside any render pass.
    std::shared_ptr<IIr77Return const> RecordCommandBuffer() {
        if (m_cmd_buffers.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: CreateCommandBuffer not called.");

        if (m_render_passes.count(Ir77PVRenderPassKind::Main) == 0 || !m_swapchain)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: main render pass or swapchain not set.");

        auto result = DefinePresentSemaphores();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        VkCommandBuffer const cmd{m_cmd_buffers[m_current_frame]};

        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkBeginCommandBuffer failed.");

        // RecordCompute(cmd), RecordAccelBuild(cmd), RecordRayTrace(cmd) go here once those paths are enabled

        result = RecordShadowPass(cmd);
        if (result->ID() == GUIDIr77OperationSucceeded) result = RecordMainPass(cmd);

        if (result->ID() != GUIDIr77OperationSucceeded) {
            vkEndCommandBuffer(cmd);
            return result;
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
    std::shared_ptr<IIr77Return const> DefineCommandPool() {
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

    std::shared_ptr<IIr77Return const> DefineCommandBuffers() {
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

        std::vector<VkFramebuffer> framebuffers{};
        m_swapchain->GetSwapchainFramebuffers(framebuffers);

        std::size_t const image_count{framebuffers.size()};

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
    struct Ir77PVPushShadow {
        std::array<float, 16> model{};
        std::uint32_t light_index{0};
    };

    struct Ir77PVPushMesh {
        std::uint32_t draw_base{0};
    };

    // must match the push ranges in Ir77PVLayout::DefineBindings
    static_assert(sizeof(Ir77PVPushShadow) == sizeof(float) * 16 + sizeof(std::uint32_t), "Shadow push range mismatch");
    static_assert(sizeof(Ir77PVPushMesh) == sizeof(std::uint32_t), "Mesh push range mismatch");

    // What is currently bound, so consecutive draws sharing state skip the rebind
    struct Ir77PVBindState {
        VkPipeline pipeline{VK_NULL_HANDLE};
        VkPipelineLayout layout{VK_NULL_HANDLE};
        VkDescriptorSet material{VK_NULL_HANDLE};
        VkDescriptorSet bones{VK_NULL_HANDLE};
        VkDescriptorSet meshlets{VK_NULL_HANDLE};
        VkBuffer vertex{VK_NULL_HANDLE};
        VkBuffer index{VK_NULL_HANDLE};
        VkIndexType index_type{VK_INDEX_TYPE_UINT32};
    };

    std::shared_ptr<IIr77Return const> RecordShadowPass(VkCommandBuffer cmd) {
        auto const& draws = m_draws[static_cast<std::size_t>(Ir77PVPass::Shadow)];

        // no shadow casters or no shadow target yet -- normal, not an error
        VkRenderPass render_pass{VK_NULL_HANDLE};
        if (draws.empty() || m_shadow_framebuffer == VK_NULL_HANDLE || !GetPass(Ir77PVRenderPassKind::Shadow, render_pass))
            return Ir77RETURN<Ir77OperationSucceeded>();

        VkClearValue clear_depth{};
        clear_depth.depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_info.renderPass = render_pass;
        begin_info.framebuffer = m_shadow_framebuffer;
        begin_info.renderArea.offset = {0, 0};
        begin_info.renderArea.extent = m_shadow_extent;
        begin_info.clearValueCount = 1;
        begin_info.pClearValues = &clear_depth;

        vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

        SetViewportScissor(cmd, m_shadow_extent);

        RecordDraws(cmd, draws);

        vkCmdEndRenderPass(cmd);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Opaque, then transparent, then the browser overlay on top
    std::shared_ptr<IIr77Return const> RecordMainPass(VkCommandBuffer cmd) {
        VkRenderPass render_pass{VK_NULL_HANDLE};
        if (!GetPass(Ir77PVRenderPassKind::Main, render_pass)) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: main render pass missing.");

        VkExtent2D extent{};
        m_swapchain->GetSwapchainExtents(extent);

        std::vector<VkFramebuffer> framebuffers{};
        m_swapchain->GetSwapchainFramebuffers(framebuffers);

        if (m_index >= framebuffers.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: image index has no framebuffer.");

        // [0] color, [1] depth
        std::array<VkClearValue, 2> clears{};
        clears[0].color = m_clear_color;
        clears[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin_info.renderPass = render_pass;
        begin_info.framebuffer = framebuffers[m_index];
        begin_info.renderArea.offset = {0, 0};
        begin_info.renderArea.extent = extent;
        begin_info.clearValueCount = static_cast<std::uint32_t>(clears.size());
        begin_info.pClearValues = clears.data();

        vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

        SetViewportScissor(cmd, extent);

        RecordDraws(cmd, m_draws[static_cast<std::size_t>(Ir77PVPass::Opaque)]);

        RecordDraws(cmd, m_draws[static_cast<std::size_t>(Ir77PVPass::Transparent)]);

        RecordOverlay(cmd);

        vkCmdEndRenderPass(cmd);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Outside any render pass. The barrier covers what compute writes for the draws: indirect commands, vertex data, storage buffers.
    std::shared_ptr<IIr77Return const> RecordCompute(VkCommandBuffer cmd) {
        if (!m_compute_pipeline || !m_compute_layout) return Ir77RETURN<Ir77OperationSucceeded>();  // no compute work -- not an error

        VkPipeline pipeline{VK_NULL_HANDLE};
        m_compute_pipeline->GetPipeline(&pipeline);

        VkPipelineLayout layout{VK_NULL_HANDLE};
        m_compute_layout->GetPipelineLayout(&layout);

        std::uint32_t set_count{0};
        m_compute_layout->GetSetCount(&set_count);

        if (m_compute_sets.size() < set_count) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: compute sets missing for the compute layout.");

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

        for (std::uint32_t i = 0; i < set_count; i++) {
            VkDescriptorSet set{VK_NULL_HANDLE};
            m_compute_sets[i]->GetSet(m_current_frame, &set);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, i, 1, &set, 0, nullptr);
        }

        vkCmdDispatch(cmd, m_compute_groups_x, m_compute_groups_y, m_compute_groups_z);

        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1, &barrier, 0,
                             nullptr, 0, nullptr);

        return Ir77RETURN<Ir77OperationSucceeded>();
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
        if (!m_ray_pipeline || !m_ray_layout || !m_trace_rays || m_ray_target == VK_NULL_HANDLE) return Ir77RETURN<Ir77OperationSucceeded>();

        VkPipeline pipeline{VK_NULL_HANDLE};
        m_ray_pipeline->GetPipeline(&pipeline);

        VkPipelineLayout layout{VK_NULL_HANDLE};
        m_ray_layout->GetPipelineLayout(&layout);

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

        for (std::uint32_t i = 0; i < m_ray_sets.size(); i++) {
            VkDescriptorSet set{VK_NULL_HANDLE};
            m_ray_sets[i]->GetSet(m_current_frame, &set);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, layout, i, 1, &set, 0, nullptr);
        }

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

    // Set 0 global and set 1 pass, rebound whenever the pipeline layout changes
    void BindFrameSets(VkCommandBuffer cmd, VkPipelineLayout layout, Ir77PVLayoutKind const& kind) {
        if (kind == Ir77PVLayoutKind::CEF) return;

        if (m_global_set) {
            VkDescriptorSet global{VK_NULL_HANDLE};
            m_global_set->GetSet(m_current_frame, &global);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &global, 0, nullptr);
        }

        if (UsesPassSet(kind) && m_pass_set) {
            VkDescriptorSet pass{VK_NULL_HANDLE};
            m_pass_set->GetSet(m_current_frame, &pass);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &pass, 0, nullptr);
        }
    }

    // Pipeline, layout and set binds are skipped when unchanged from the previous item.
    // Mesh items pull geometry from set 3 and dispatch task groups; everything else binds vertex / index and draws.
    void RecordDraws(VkCommandBuffer cmd, std::vector<Ir77PVDrawItem> const& draws) {
        Ir77PVBindState state{};

        for (auto const& item : draws) {
            if (!item.pipeline || !item.layout) continue;

            VkPipeline pipeline{VK_NULL_HANDLE};
            item.pipeline->GetPipeline(&pipeline);

            VkPipelineLayout layout{VK_NULL_HANDLE};
            item.layout->GetPipelineLayout(&layout);

            if (pipeline == VK_NULL_HANDLE || layout == VK_NULL_HANDLE) continue;

            Ir77PVLayoutKind kind{Ir77PVLayoutKind::Static};
            item.layout->GetKind(&kind);

            bool const mesh{IsMeshKind(kind)};

            // mesh items need the extension loaded and a meshlet set; raster items need a vertex buffer
            if (mesh ? (!m_draw_mesh_tasks || !item.meshlets) : !item.vertex) continue;

            if (pipeline != state.pipeline) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                state.pipeline = pipeline;
            }

            // a new layout invalidates every set bound above 0 / 1
            if (layout != state.layout) {
                BindFrameSets(cmd, layout, kind);
                state.layout = layout;
                state.material = VK_NULL_HANDLE;
                state.bones = VK_NULL_HANDLE;
                state.meshlets = VK_NULL_HANDLE;
            }

            if (UsesMaterialSet(kind) && item.material) BindSetIfChanged(cmd, layout, 2, item.material, state.material);

            // bones -- set 3 Skinned, set 1 ShadowSkinned
            std::uint32_t const bones_index{BonesSetIndex(kind)};
            if (bones_index != NO_SET && item.bones) BindSetIfChanged(cmd, layout, bones_index, item.bones, state.bones);

            if (mesh) {
                RecordMeshDraw(cmd, layout, item, state);
                continue;
            }

            RecordRasterDraw(cmd, layout, kind, item, state);
        }
    }

    // Set 3 meshlets, task/mesh push constant, no vertex / index binding
    void RecordMeshDraw(VkCommandBuffer cmd, VkPipelineLayout layout, Ir77PVDrawItem const& item, Ir77PVBindState& state) {
        BindSetIfChanged(cmd, layout, 3, item.meshlets, state.meshlets);

        Ir77PVPushMesh push{};
        push.draw_base = item.draw_base;

        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, 0, sizeof(Ir77PVPushMesh), &push);

        if (item.indirect) {
            VkBuffer indirect{VK_NULL_HANDLE};
            item.indirect->GetBuffer(m_current_frame, &indirect);

            m_draw_mesh_tasks_indirect(cmd, indirect, item.indirect_offset, item.indirect_count, sizeof(VkDrawMeshTasksIndirectCommandEXT));
        } else {
            m_draw_mesh_tasks(cmd, (item.meshlet_count + TASK_GROUP_SIZE - 1) / TASK_GROUP_SIZE, 1, 1);
        }
    }

    // Model push constant, vertex / optional index binding, then direct or indirect draw. Without an index buffer, index_count is the vertex count.
    void RecordRasterDraw(VkCommandBuffer cmd, VkPipelineLayout layout, Ir77PVLayoutKind const& kind, Ir77PVDrawItem const& item, Ir77PVBindState& state) {
        if (IsShadowKind(kind)) {
            Ir77PVPushShadow push{};
            push.model = item.model;
            push.light_index = item.light_index;

            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Ir77PVPushShadow), &push);
        } else {
            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16, item.model.data());
        }

        VkBuffer vertex{VK_NULL_HANDLE};
        item.vertex->GetBuffer(m_current_frame, &vertex);

        if (vertex != state.vertex) {
            VkDeviceSize const offset{0};
            vkCmdBindVertexBuffers(cmd, 0, 1, &vertex, &offset);
            state.vertex = vertex;
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
    void RecordOverlay(VkCommandBuffer cmd) {
        if (!m_overlay || !m_overlay_pipeline || !m_overlay_layout) return;

        bool resizing{false};
        m_overlay->IsResizing(resizing);

        bool ever_uploaded{false};
        m_overlay->HasEverUploaded(ever_uploaded);

        if (resizing || !ever_uploaded) return;

        VkPipeline pipeline{VK_NULL_HANDLE};
        m_overlay_pipeline->GetPipeline(&pipeline);

        VkPipelineLayout layout{VK_NULL_HANDLE};
        m_overlay_layout->GetPipelineLayout(&layout);

        VkDescriptorSet set{VK_NULL_HANDLE};
        m_overlay->GetDescriptorSet(&set);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, nullptr);
        vkCmdDraw(cmd, OVERLAY_VERTICES, 1, 0, 0);
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // internals
    // -------------------------------------------------------------------------------------------------------------------------------------
   private:
    bool GetPass(Ir77PVRenderPassKind const& kind, VkRenderPass& render_pass) const {
        auto const found = m_render_passes.find(kind);
        if (found == m_render_passes.end() || !found->second) return false;

        found->second->GetRenderPass(&render_pass);

        return render_pass != VK_NULL_HANDLE;
    }

    void BindSetIfChanged(VkCommandBuffer cmd, VkPipelineLayout layout, std::uint32_t const set_index, std::shared_ptr<IIr77PVDescriptorSet> const& source,
                          VkDescriptorSet& bound) {
        VkDescriptorSet set{VK_NULL_HANDLE};
        source->GetSet(m_current_frame, &set);

        if (set == bound) return;

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, set_index, 1, &set, 0, nullptr);
        bound = set;
    }

    void SetViewportScissor(VkCommandBuffer cmd, VkExtent2D const& extent) const {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;

        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    bool IsShadowKind(Ir77PVLayoutKind const& kind) const { return kind == Ir77PVLayoutKind::Shadow || kind == Ir77PVLayoutKind::ShadowSkinned; }

    bool IsMeshKind(Ir77PVLayoutKind const& kind) const { return kind == Ir77PVLayoutKind::Mesh; }

    // set 1 (pass) and set 2 (material) exist only in the main-pass lit layouts
    bool UsesPassSet(Ir77PVLayoutKind const& kind) const {
        return kind == Ir77PVLayoutKind::Static || kind == Ir77PVLayoutKind::Skinned || kind == Ir77PVLayoutKind::Mesh;
    }

    bool UsesMaterialSet(Ir77PVLayoutKind const& kind) const { return UsesPassSet(kind); }

    // NO_SET = the layout has no bones set
    std::uint32_t BonesSetIndex(Ir77PVLayoutKind const& kind) const {
        switch (kind) {
            case Ir77PVLayoutKind::Skinned:
                return 3;

            case Ir77PVLayoutKind::ShadowSkinned:
                return 1;

            default:
                return NO_SET;
        }
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

    static constexpr std::uint32_t NO_SET = UINT32_MAX;

    // one draw list per Ir77PVPass: Shadow, Opaque, Transparent
    static constexpr std::size_t PASS_COUNT = 3;

    std::shared_ptr<IIr77PVDevice> m_device{};

    std::shared_ptr<IIr77PVSwapchain> m_swapchain{};

    std::map<Ir77PVRenderPassKind, std::shared_ptr<IIr77PVRenderPass>> m_render_passes{};

    VkFramebuffer m_shadow_framebuffer{VK_NULL_HANDLE};

    VkExtent2D m_shadow_extent{};

    std::shared_ptr<IIr77PVDescriptorSet> m_global_set{};

    std::shared_ptr<IIr77PVDescriptorSet> m_pass_set{};

    std::shared_ptr<IIr77PVOverlay> m_overlay{};

    std::shared_ptr<IIr77PVPipeline> m_overlay_pipeline{};

    std::shared_ptr<IIr77PVLayout> m_overlay_layout{};

    VkClearColorValue m_clear_color{{0.0f, 0.0f, 0.0f, 1.0f}};

    std::array<std::vector<Ir77PVDrawItem>, PASS_COUNT> m_draws{};

    // compute
    std::shared_ptr<IIr77PVPipelineCPT> m_compute_pipeline{};

    std::shared_ptr<IIr77PVLayoutCPT> m_compute_layout{};

    std::vector<std::shared_ptr<IIr77PVDescriptorSet>> m_compute_sets{};

    std::uint32_t m_compute_groups_x{1};

    std::uint32_t m_compute_groups_y{1};

    std::uint32_t m_compute_groups_z{1};

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

    std::shared_ptr<IIr77PVLayoutRT> m_ray_layout{};

    std::vector<std::shared_ptr<IIr77PVDescriptorSet>> m_ray_sets{};

    VkImage m_ray_target{VK_NULL_HANDLE};

    VkExtent2D m_ray_extent{};

    // TLAS here, BLASes owned by Asset
    std::shared_ptr<IIr77PVAccelStructure> m_tlas{};

    PFN_vkCmdTraceRaysKHR m_trace_rays{nullptr};

    PFN_vkCmdBuildAccelerationStructuresKHR m_build_accel{nullptr};
    */
};

}  // namespace NSIr77PeregrineV