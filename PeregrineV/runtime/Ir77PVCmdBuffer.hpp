#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVCmdBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

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

    ~Ir77PVCmdBuffer() {
        if (!m_device) return;

        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        for (std::size_t i = 0; i < m_fences_in_flight.size(); i++) {
            vkDestroySemaphore(device, m_semaphores_available[i], nullptr);
            vkDestroySemaphore(device, m_semaphores_finished[i], nullptr);
            vkDestroyFence(device, m_fences_in_flight[i], nullptr);
        }

        vkDestroyCommandPool(device, m_cmd_pool, nullptr);
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

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77PVCmdBuffer)
            obj = std::shared_ptr<IIr77PVCmdBuffer>(shared_from_this(), static_cast<IIr77PVCmdBuffer*>(this));

        else if (iid == &GUIDIr77PVCmdBuffer)
            obj = std::shared_ptr<Ir77PVCmdBuffer>(shared_from_this(), static_cast<Ir77PVCmdBuffer*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // wiring
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetInstance(std::shared_ptr<IIr77PVInstance> instance) {
        m_instance = instance;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetSwapchain(std::shared_ptr<IIr77PVSwapchain> swapchain) {
        m_swapchain = swapchain;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetRenderPass(std::shared_ptr<IIr77PVRenderPass> render_pass) {
        m_render_pass = render_pass;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Depth-only pass the Shadow / ShadowSkinned pipelines were built against, plus its shadow-map framebuffer
    std::shared_ptr<IIr77Return const> SetShadowTarget(std::shared_ptr<IIr77PVRenderPass> render_pass, VkFramebuffer const& framebuffer,
                                                       VkExtent2D const& extent) {
        m_shadow_render_pass = render_pass;
        m_shadow_framebuffer = framebuffer;
        m_shadow_extent = extent;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Set 0 -- camera, lights, instances, env map. Bound once per pipeline-layout change.
    std::shared_ptr<IIr77Return const> SetGlobalSet(std::shared_ptr<IIr77PVDescriptorSet> global_set) {
        m_global_set = global_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Set 1 -- shadow map + shadow matrices. Main-pass Static / Skinned only.
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
        std::size_t const slot = static_cast<std::size_t>(pass);
        if (slot >= m_draws.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: unknown pass.");

        m_draws[slot] = draws;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> ClearDraws() {
        for (auto& list : m_draws) list.clear();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // setup
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> DefineSyncObjects() {
        VkDevice device;
        m_device->GetDevice(&device);

        m_semaphores_available.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
        m_semaphores_finished.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
        m_fences_in_flight.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (std::uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphore_info, nullptr, &m_semaphores_available[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphore_info, nullptr, &m_semaphores_finished[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fence_info, nullptr, &m_fences_in_flight[i]) != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: sync object creation failed.");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Also resolves and caches the presentation queue, so the pool, submit and present all use the same family
    std::shared_ptr<IIr77Return const> DefineCommandPool() {
        VkDevice device;
        m_device->GetDevice(&device);

        std::vector<Ir77PVQueueFamily> queue_families{};
        m_device->GetQueueFamilies(queue_families);

        bool found = false;
        for (std::size_t i = 0; i < queue_families.size(); i++) {
            if (queue_families[i].presentation == VK_TRUE) {
                m_queue = queue_families[i].queue;
                m_queue_family = static_cast<std::uint32_t>(i);
                found = true;
                break;
            }
        }

        if (!found) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: no presentation queue.");

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = m_queue_family;

        if (vkCreateCommandPool(device, &pool_info, nullptr, &m_cmd_pool) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkCreateCommandPool failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> AllocateCommandBuffer() {
        VkDevice device;
        m_device->GetDevice(&device);

        m_cmd_buffers.assign(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = m_cmd_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        if (vkAllocateCommandBuffers(device, &alloc_info, m_cmd_buffers.data()) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkAllocateCommandBuffers failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // frame loop: WaitForFence -> (update Dynamic buffers) -> AcquireNextImage -> ResetFence -> RecordCommandBuffer -> SubmitFrame -> PresentFrame
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> WaitForFence() {
        VkDevice device;
        m_device->GetDevice(&device);

        if (vkWaitForFences(device, 1, &m_fences_in_flight[m_current_frame], VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkWaitForFences failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> AcquireNextImage(VkResult* acquire_next_result) {
        VkDevice device;
        m_device->GetDevice(&device);

        VkSwapchainKHR swapchain;
        m_swapchain->GetSwapchain(&swapchain);

        *acquire_next_result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, m_semaphores_available[m_current_frame], VK_NULL_HANDLE, &m_index);

        if (*acquire_next_result != VK_SUCCESS && *acquire_next_result != VK_SUBOPTIMAL_KHR && *acquire_next_result != VK_ERROR_OUT_OF_DATE_KHR) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkAcquireNextImageKHR failed unexpectedly.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Only after a successful acquire -- resetting then bailing on OUT_OF_DATE deadlocks the next WaitForFence
    std::shared_ptr<IIr77Return const> ResetFence() {
        VkDevice device;
        m_device->GetDevice(&device);

        vkResetFences(device, 1, &m_fences_in_flight[m_current_frame]);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> RecordCommandBuffer() {
        if (m_cmd_buffers.empty()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: AllocateCommandBuffer not called.");
        if (!m_render_pass || !m_swapchain) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: render pass or swapchain not set.");

        VkCommandBuffer const cmd = m_cmd_buffers[m_current_frame];

        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkBeginCommandBuffer failed.");
        }

        RecordShadowPass(cmd);

        auto const main = RecordMainPass(cmd);
        if (main->ID() != &GUIDIr77OperationSucceeded) {
            vkEndCommandBuffer(cmd);
            return main;
        }

        if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkEndCommandBuffer failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SubmitFrame() {
        VkSemaphore const wait_semaphores[] = {m_semaphores_available[m_current_frame]};
        VkPipelineStageFlags const wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSemaphore const signal_semaphores[] = {m_semaphores_finished[m_current_frame]};

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_cmd_buffers[m_current_frame];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        if (vkQueueSubmit(m_queue, 1, &submit_info, m_fences_in_flight[m_current_frame]) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: vkQueueSubmit failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> PresentFrame(VkResult* queue_present_result) {
        VkSwapchainKHR swapchain;
        m_swapchain->GetSwapchain(&swapchain);

        VkSemaphore const wait_semaphores[] = {m_semaphores_finished[m_current_frame]};

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = wait_semaphores;
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

    // For Ir77PVBuffer::Upload at load time
    std::shared_ptr<IIr77Return const> GetCommandPool(VkCommandPool* pool) {
        *pool = m_cmd_pool;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetQueue(VkQueue* queue) {
        *queue = m_queue;

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

    // What is currently bound, so consecutive draws sharing state skip the rebind
    struct Ir77PVBindState {
        VkPipeline pipeline{VK_NULL_HANDLE};

        VkPipelineLayout layout{VK_NULL_HANDLE};

        VkDescriptorSet material{VK_NULL_HANDLE};

        VkDescriptorSet bones{VK_NULL_HANDLE};

        VkBuffer vertex{VK_NULL_HANDLE};

        VkBuffer index{VK_NULL_HANDLE};

        VkIndexType index_type{VK_INDEX_TYPE_UINT32};
    };

    static bool IsShadowKind(Ir77PVLayoutKind const& kind) { return kind == Ir77PVLayoutKind::Shadow || kind == Ir77PVLayoutKind::ShadowSkinned; }

    static bool UsesPassSet(Ir77PVLayoutKind const& kind) { return kind == Ir77PVLayoutKind::Static || kind == Ir77PVLayoutKind::Skinned; }

    static bool UsesMaterialSet(Ir77PVLayoutKind const& kind) { return kind == Ir77PVLayoutKind::Static || kind == Ir77PVLayoutKind::Skinned; }

    // UINT32_MAX = layout has no bones set
    static std::uint32_t BonesSetIndex(Ir77PVLayoutKind const& kind) {
        switch (kind) {
            case Ir77PVLayoutKind::Skinned:
                return 3;
            case Ir77PVLayoutKind::ShadowSkinned:
                return 1;
            default:
                return UINT32_MAX;
        }
    }

    static void SetViewportScissor(VkCommandBuffer cmd, VkExtent2D const& extent) {
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

    void RecordShadowPass(VkCommandBuffer cmd) {
        auto const& draws = m_draws[static_cast<std::size_t>(Ir77PVPass::Shadow)];

        if (draws.empty() || !m_shadow_render_pass || m_shadow_framebuffer == VK_NULL_HANDLE) return;

        VkRenderPass render_pass;
        m_shadow_render_pass->GetRenderPass(&render_pass);

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
    }

    std::shared_ptr<IIr77Return const> RecordMainPass(VkCommandBuffer cmd) {
        VkRenderPass render_pass;
        m_render_pass->GetRenderPass(&render_pass);

        VkExtent2D extent;
        m_swapchain->GetSwapchainExtents(extent);

        std::vector<VkFramebuffer> framebuffers{};
        m_swapchain->GetSwapchainFramebuffers(framebuffers);

        if (m_index >= framebuffers.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: image index has no framebuffer.");

        // [0] color, [1] depth -- the depth clear is ignored until the render pass has a depth attachment
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

    void RecordDraws(VkCommandBuffer cmd, std::vector<Ir77PVDrawItem> const& draws) {
        Ir77PVBindState state{};

        for (auto const& item : draws) {
            if (!item.pipeline || !item.layout || !item.vertex) continue;

            VkPipeline pipeline{VK_NULL_HANDLE};
            item.pipeline->GetPipeline(&pipeline);

            VkPipelineLayout layout{VK_NULL_HANDLE};
            item.layout->GetPipelineLayout(&layout);

            Ir77PVLayoutKind kind{Ir77PVLayoutKind::Static};
            item.layout->GetKind(&kind);

            if (pipeline == VK_NULL_HANDLE || layout == VK_NULL_HANDLE) continue;

            // pipeline
            if (pipeline != state.pipeline) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                state.pipeline = pipeline;
            }

            // frame sets -- a new layout invalidates everything bound above set 0/1
            if (layout != state.layout) {
                BindFrameSets(cmd, layout, kind);
                state.layout = layout;
                state.material = VK_NULL_HANDLE;
                state.bones = VK_NULL_HANDLE;
            }

            // set 2 material
            if (UsesMaterialSet(kind) && item.material) {
                VkDescriptorSet material{VK_NULL_HANDLE};
                item.material->GetSet(m_current_frame, &material);

                if (material != state.material) {
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, &material, 0, nullptr);
                    state.material = material;
                }
            }

            // bones (set 3 Skinned, set 1 ShadowSkinned)
            std::uint32_t const bones_index = BonesSetIndex(kind);
            if (bones_index != UINT32_MAX && item.bones) {
                VkDescriptorSet bones{VK_NULL_HANDLE};
                item.bones->GetSet(m_current_frame, &bones);

                if (bones != state.bones) {
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, bones_index, 1, &bones, 0, nullptr);
                    state.bones = bones;
                }
            }

            // push constants -- sizes match Ir77PVLayout::SetKind
            if (IsShadowKind(kind)) {
                Ir77PVPushShadow push{};
                push.model = item.model;
                push.light_index = item.light_index;

                vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16 + sizeof(std::uint32_t), &push);
            } else {
                vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16, item.model.data());
            }

            // vertex
            VkBuffer vertex{VK_NULL_HANDLE};
            item.vertex->GetBuffer(m_current_frame, &vertex);

            if (vertex != state.vertex) {
                VkDeviceSize const offset = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &vertex, &offset);
                state.vertex = vertex;
            }

            // index (optional -- without one, index_count is used as the vertex count)
            bool const indexed = static_cast<bool>(item.index);

            if (indexed) {
                VkBuffer index{VK_NULL_HANDLE};
                item.index->GetBuffer(m_current_frame, &index);

                if (index != state.index || item.index_type != state.index_type) {
                    vkCmdBindIndexBuffer(cmd, index, 0, item.index_type);
                    state.index = index;
                    state.index_type = item.index_type;
                }
            }

            // draw
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
    }

    // Browser composite, drawn last over the scene
    void RecordOverlay(VkCommandBuffer cmd) {
        if (!m_overlay || !m_overlay_pipeline || !m_overlay_layout) return;

        bool resizing = false;
        bool ever_uploaded = false;
        m_overlay->IsResizing(resizing);
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
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }

   private:
    std::uint32_t m_index{0};

    std::uint32_t m_current_frame{0};

    std::shared_ptr<IIr77PVInstance> m_instance;

    std::shared_ptr<IIr77PVDevice> m_device;

    std::shared_ptr<IIr77PVSwapchain> m_swapchain;

    std::shared_ptr<IIr77PVRenderPass> m_render_pass;

    std::shared_ptr<IIr77PVRenderPass> m_shadow_render_pass;

    VkFramebuffer m_shadow_framebuffer{VK_NULL_HANDLE};

    VkExtent2D m_shadow_extent{};

    std::shared_ptr<IIr77PVDescriptorSet> m_global_set;

    std::shared_ptr<IIr77PVDescriptorSet> m_pass_set;

    std::shared_ptr<IIr77PVOverlay> m_overlay;

    std::shared_ptr<IIr77PVPipeline> m_overlay_pipeline;

    std::shared_ptr<IIr77PVLayout> m_overlay_layout;

    VkClearColorValue m_clear_color{{0.0f, 0.0f, 0.0f, 1.0f}};

    std::array<std::vector<Ir77PVDrawItem>, 3> m_draws{};

    VkQueue m_queue{VK_NULL_HANDLE};

    std::uint32_t m_queue_family{0};

    std::vector<VkSemaphore> m_semaphores_available{};

    std::vector<VkSemaphore> m_semaphores_finished{};

    std::vector<VkFence> m_fences_in_flight{};

    VkCommandPool m_cmd_pool{VK_NULL_HANDLE};

    std::vector<VkCommandBuffer> m_cmd_buffers{};
};
}  // namespace NSIr77PeregrineV