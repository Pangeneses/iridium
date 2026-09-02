#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#include <vector>

#include "../Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../dictionary/IDIIr77PeregrineV.hpp"
#include "../../dictionary/IDIr77PeregrineV.hpp"

#include "../../dictionary/IDIr77PVContext.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../../interface/IIr77PVCmdBuffer.hpp"

#include "../../runtime/Buffer/Ir77PVBufferCEF.hpp"
#include "../../runtime/Buffer/Ir77PVBufferUBO.hpp"
#include "../../runtime/Buffer/Ir77PVBufferVertex.hpp"

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
        VkDevice device;
        m_device->GetDevice(&device);

        vkDeviceWaitIdle(device);

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
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

   public:
   std::shared_ptr<IIr77Return const> SetCurrentFrame(std::uint32_t const& current_frame) {
        m_current_frame = current_frame;

        return Ir77RETURN<Ir77OperationSucceeded>();
   }

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

    std::shared_ptr<IIr77Return const> SetPipelineGFX(std::shared_ptr<IIr77PVPipeline> pipeline_gfx) {
        m_pipeline_gfx = pipeline_gfx;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetBufferVertex(std::shared_ptr<Ir77PVBufferVertex> buffer_vertex) {
        m_buffer_vertex = buffer_vertex;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetLayoutUBO(std::shared_ptr<IIr77PVLayout> layout_ubo) {
        m_layout_ubo = layout_ubo;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetBufferUBO(std::shared_ptr<Ir77PVBufferUBO> buffer_ubo) {
        m_buffer_ubo = buffer_ubo;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetPipelineCEF(std::shared_ptr<IIr77PVPipeline> pipeline_cef) {
        m_pipeline_cef = pipeline_cef;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetLayoutCEF(std::shared_ptr<IIr77PVLayout> layout_cef) {
        m_layout_cef = layout_cef;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetBufferCEF(std::shared_ptr<Ir77PVBufferCEF> buffer_cef) {
        m_buffer_cef = buffer_cef;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineSyncObjects() {
        VkDevice device;
        m_device->GetDevice(&device);

        m_semaphores_available.resize(MAX_FRAMES_IN_FLIGHT);
        m_semaphores_finished.resize(MAX_FRAMES_IN_FLIGHT);
        m_fences_in_flight.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphore_info, nullptr, &m_semaphores_available[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphore_info, nullptr, &m_semaphores_finished[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fence_info, nullptr, &m_fences_in_flight[i]) != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: sync object creation failed.");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DefineCommandPool() {
        VkDevice device;
        m_device->GetDevice(&device);

        std::vector<Ir77PVQueueFamily> queue_family;
        m_device->GetQueueFamilies(queue_family);

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        pool_info.queueFamilyIndex = UINT32_MAX;

        for (int i = 0; i < queue_family.size(); i++) {
            if (queue_family[i].presentation == VK_TRUE) {
                pool_info.queueFamilyIndex = i;
                break;
            }
        }

        if (pool_info.queueFamilyIndex == UINT32_MAX) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCmdBuffer: no presentation queue.");

        if (vkCreateCommandPool(device, &pool_info, nullptr, &m_cmd_pool) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkCreateCommandPool failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> AllocateCommandBuffer() {
        VkDevice device;
        m_device->GetDevice(&device);

        m_cmd_buffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = m_cmd_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

        if (vkAllocateCommandBuffers(device, &alloc_info, m_cmd_buffers.data()) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkAllocateCommandBuffers failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> WaitForFence() {
        VkDevice device;
        m_device->GetDevice(&device);

        if (vkWaitForFences(device, 1, &m_fences_in_flight[m_current_frame], VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkWaitForFences failed.");
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
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkAcquireNextImageKHR failed unexpectedly.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> ResetFence() {
        VkDevice device;
        m_device->GetDevice(&device);

        vkResetFences(device, 1, &m_fences_in_flight[m_current_frame]);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> RecordCommandBuffer() {
        VkDevice device;
        m_device->GetDevice(&device);

        VkRenderPass render_pass;
        m_render_pass->GetRenderPass(&render_pass);

        VkExtent2D swapchain_extent;
        m_swapchain->GetSwapchainExtents(swapchain_extent);

        VkCommandBuffer cmd_buffer = m_cmd_buffers[m_current_frame];

        std::vector<VkFramebuffer> swapchain_framebuffers;
        m_swapchain->GetSwapchainFramebuffers(swapchain_framebuffers);

        VkPipeline pipeline_gfx;
        m_pipeline_gfx->GetPipeline(&pipeline_gfx);

        VkBuffer vertex_buffer;
        m_buffer_vertex->GetBuffer(&vertex_buffer);

        VkBuffer index_buffer;
        m_buffer_vertex->GetIndexBuffer(&index_buffer);

        std::uint32_t index_count;
        m_buffer_vertex->GetIndexCount(index_count);

        VkIndexType index_type;
        m_buffer_vertex->GetIndexType(index_type);

        VkDescriptorSet ubo_descriptor_set;
        m_buffer_ubo->GetDescriptorSet(m_current_frame, &ubo_descriptor_set);

        VkPipelineLayout pipeline_layout_ubo;
        m_layout_ubo->GetPipelineLayout(&pipeline_layout_ubo);

        // new — CEF pipeline + its pipeline layout + this window's descriptor set
        VkPipeline pipeline_cef = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout_cef = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set_cef = VK_NULL_HANDLE;
        bool has_cef = m_pipeline_cef && m_layout_cef && m_buffer_cef;

        if (has_cef) {
            m_pipeline_cef->GetPipeline(&pipeline_cef);
            m_layout_cef->GetPipelineLayout(&pipeline_layout_cef);
            m_buffer_cef->GetDescriptorSet(&descriptor_set_cef);
        }

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(cmd_buffer, &begin_info) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkBeginCommandBuffer failed.");
        }

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass;
        render_pass_info.framebuffer = swapchain_framebuffers[m_index];
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = swapchain_extent;

        VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchain_extent.width);
        viewport.height = static_cast<float>(swapchain_extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapchain_extent;
        vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_gfx);

        VkBuffer vertex_buffers[] = {vertex_buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, vertex_buffers, offsets);

        vkCmdBindIndexBuffer(cmd_buffer, index_buffer, 0, index_type);

        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_ubo, 0, 1, &ubo_descriptor_set, 0, nullptr);

        vkCmdDrawIndexed(cmd_buffer, index_count, 1, 0, 0, 0);

        bool resizing = false;
        bool ever_uploaded = false;
        if (has_cef) {
            m_buffer_cef->IsResizing(resizing);
            m_buffer_cef->HasEverUploaded(ever_uploaded);
        }
        if (has_cef && !resizing && ever_uploaded) {
            vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_cef);
            vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_cef, 0, 1, &descriptor_set_cef, 0, nullptr);
            vkCmdDraw(cmd_buffer, 6, 1, 0, 0);
        }

        vkCmdEndRenderPass(cmd_buffer);

        if (vkEndCommandBuffer(cmd_buffer) != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkEndCommandBuffer failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SubmitFrame() {
        VkDevice device;
        m_device->GetDevice(&device);

        std::vector<Ir77PVQueueFamily> queue_families;
        m_device->GetQueueFamilies(queue_families);

        VkQueue graphics_queue{VK_NULL_HANDLE};
        for (int i = 0; i < queue_families.size(); i++) {
            if (queue_families[i].presentation == true) graphics_queue = queue_families[i].queue;
        }
        if (!graphics_queue) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77CmdBuffer: no presentation queue.");

        std::vector<VkSemaphore> wait_semaphores = {m_semaphores_available[m_current_frame]};
        std::vector<VkPipelineStageFlags> wait_stages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSemaphore signal_semaphores[] = {m_semaphores_finished[m_current_frame]};

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
        submit_info.pWaitSemaphores = wait_semaphores.data();
        submit_info.pWaitDstStageMask = wait_stages.data();
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &m_cmd_buffers[m_current_frame];
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        VkResult result = vkQueueSubmit(graphics_queue, 1, &submit_info, m_fences_in_flight[m_current_frame]);
        if (result != VK_SUCCESS) {
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: vkQueueSubmit failed.");
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> PresentFrame(VkResult* queue_present_result) {
        VkSwapchainKHR swapchain;
        m_swapchain->GetSwapchain(&swapchain);

        std::vector<Ir77PVQueueFamily> queue_families;
        m_device->GetQueueFamilies(queue_families);

        VkQueue graphics_queue{VK_NULL_HANDLE};
        for (int i = 0; i < queue_families.size(); i++) {
            if (queue_families[i].presentation == true) graphics_queue = queue_families[i].queue;
        }
        if (!graphics_queue) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77CmdBuffer: no presentation queue.");

        VkSemaphore wait_semaphores[] = {m_semaphores_finished[m_current_frame]};

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain;
        present_info.pImageIndices = &m_index;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = wait_semaphores;

        *queue_present_result = vkQueuePresentKHR(graphics_queue, &present_info);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::uint32_t m_index{0};

    std::uint32_t m_current_frame{0};

    std::shared_ptr<IIr77PVInstance> m_instance;

    std::shared_ptr<IIr77PVDevice> m_device;

    std::shared_ptr<IIr77PVSwapchain> m_swapchain;

    std::shared_ptr<IIr77PVRenderPass> m_render_pass;

    std::shared_ptr<IIr77PVPipeline> m_pipeline_gfx;

    std::shared_ptr<IIr77PVPipeline> m_pipeline_cef;

    std::shared_ptr<Ir77PVBufferVertex> m_buffer_vertex;

    std::shared_ptr<IIr77PVLayout> m_layout_ubo;

    std::shared_ptr<Ir77PVBufferUBO> m_buffer_ubo;

    std::shared_ptr<IIr77PVLayout> m_layout_cef;

    std::shared_ptr<Ir77PVBufferCEF> m_buffer_cef;

    std::vector<VkSemaphore> m_semaphores_available;

    std::vector<VkSemaphore> m_semaphores_finished;

    std::vector<VkFence> m_fences_in_flight;

    VkCommandPool m_cmd_pool{VK_NULL_HANDLE};

    std::vector<VkCommandBuffer> m_cmd_buffers;
};
}  // namespace NSIr77PeregrineV