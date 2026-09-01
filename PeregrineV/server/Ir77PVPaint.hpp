#pragma once

#include <SDL3/SDL_video.h>

#include <map>
#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "IIr77PVPaint.hpp"

#include "Ir77PeregrineV.hpp"

#include "../runtime/GPU/Ir77PVCmdBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVPaint : public Ir77Enlisted, public IIr77PVPaint, public std::enable_shared_from_this<Ir77PVPaint> {
   public:
    Ir77PVPaint() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PVPaint>(uid);

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

        else if (iid == &GUIDIIr77PVPaint)
            obj = std::shared_ptr<IIr77PVPaint>(shared_from_this(), static_cast<IIr77PVPaint*>(this));

        else if (iid == &GUIDIr77PVPaint)
            obj = std::shared_ptr<Ir77PVPaint>(shared_from_this(), static_cast<Ir77PVPaint*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetPeregrineV(std::shared_ptr<Ir77PeregrineV>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateCommandBuffers() {
        std::vector<SDL_Window*> windows = m_context->m_windows.at(m_context->m_current_device);

        if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

        std::vector<std::shared_ptr<IIr77PVCmdBuffer>> cmd_buffers;
        for (int i = 0; i < windows.size(); i++) {
            auto cmd_buffer = std::static_pointer_cast<IIr77PVCmdBuffer>(std::make_shared<Ir77PVCmdBuffer>());

            cmd_buffer->SetInstance(m_context->m_instance);

            cmd_buffer->SetDevice(m_context->m_devices.at(m_context->m_current_device));

            cmd_buffer->SetSwapchain(m_context->m_swapchains.at(m_context->m_current_device).at(i));

            cmd_buffer->SetRenderPass(m_context->m_render_pass.at(m_context->m_current_device));

            cmd_buffer->SetPipelineGFX(m_context->m_pipelines_gfx.at(m_context->m_current_device).at(i));
           
            cmd_buffer->SetBufferVertex(m_context->m_buffer_vertex.at(m_context->m_current_device).at(i));

            cmd_buffer->SetLayoutUBO(m_context->m_layouts_ubo.at(m_context->m_current_device));

            cmd_buffer->SetBufferUBO(m_context->m_buffer_ubo.at(m_context->m_current_device).at(i));

            cmd_buffer->SetPipelineCEF(m_context->m_pipelines_cef.at(m_context->m_current_device).at(i));

            cmd_buffer->SetLayoutCEF(m_context->m_layouts_cef.at(m_context->m_current_device));

            cmd_buffer->SetBufferCEF(m_context->m_buffer_cef.at(m_context->m_current_device).at(i));

            cmd_buffer->DefineSyncObjects();

            cmd_buffer->DefineCommandPool();

            cmd_buffer->AllocateCommandBuffer();

            cmd_buffers.push_back(cmd_buffer);
        }

        m_context->m_command_buffers.emplace(m_context->m_current_device, cmd_buffers);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Command Buffers.");
    }

    std::shared_ptr<IIr77Return const> Draw() {
        if (m_context->m_resize_in_progress) return Ir77RETURN<Ir77OperationSucceeded>();

        std::vector<std::shared_ptr<IIr77PVCmdBuffer>> cmd_buffers = m_context->m_command_buffers.at(m_context->m_current_device);
        for (int i = 0; i < cmd_buffers.size(); i++) {
            cmd_buffers[i]->WaitForFence();

            VkResult acquire_result;
            cmd_buffers[i]->AcquireNextImage(&acquire_result);

            if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR) {
                ResetSwapchain(i);
                continue;
            } else if (acquire_result != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: pipeline corrupted.");
            }

            cmd_buffers[i]->ResetFence();
            cmd_buffers[i]->RecordCommandBuffer();
            cmd_buffers[i]->SubmitFrame();

            VkResult present_result;
            cmd_buffers[i]->PresentFrame(&present_result);

            if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
                ResetSwapchain(i);
            } else if (present_result != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: pipeline corrupted.");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Draw.");
    }

std::shared_ptr<IIr77Return const> ResetSwapchain(std::uint32_t const& index) {
    m_context->m_resize_in_progress = true;

    m_context->m_buffer_cef.at(m_context->m_current_device).at(index)->SetResizing(true);

    VkDevice device;
    m_context->m_devices.at(m_context->m_current_device)->GetDevice(&device);
    vkDeviceWaitIdle(device);

    std::vector<std::shared_ptr<IIr77PVSwapchain>> swapchains = m_context->m_swapchains.at(m_context->m_current_device);

    if (swapchains.at(index)->QuerySwapchainSupport()->ID() != &GUIDIr77OperationSucceeded)
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: QuerySwapchainSupport failed.");

    if (swapchains.at(index)->SwapSurfaceFormat()->ID() != &GUIDIr77OperationSucceeded)
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SwapSurfaceFormat failed.");

    if (swapchains.at(index)->PresentMode()->ID() != &GUIDIr77OperationSucceeded)
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: PresentMode failed.");

    if (swapchains.at(index)->SurfaceCapabilities()->ID() != &GUIDIr77OperationSucceeded)
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SurfaceCapabilities failed.");

    if (swapchains.at(index)->DefineSwapchain()->ID() != &GUIDIr77OperationSucceeded)
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: DefineSwapchain failed.");

    if (swapchains.at(index)->CleanupSwapchain()->ID() != &GUIDIr77OperationSucceeded)
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: CleanupSwapchain failed.");

    if (swapchains.at(index)->InitSwapchainImages()->ID() != &GUIDIr77OperationSucceeded)
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: InitSwapchainImages failed.");

    if (swapchains.at(index)->DefineImageView()->ID() != &GUIDIr77OperationSucceeded)
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: DefineImageView failed.");

    if (swapchains.at(index)->DefineFramebuffers()->ID() != &GUIDIr77OperationSucceeded)
        return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: DefineFramebuffers failed.");

    m_context->m_buffer_cef.at(m_context->m_current_device).at(index)->SetResizing(false);

    m_context->m_resize_in_progress = false;

    return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: ValidateSwapchain.");
}

   private:
    std::shared_ptr<Ir77PeregrineV> m_context{nullptr};
};
}  // namespace NSIr77PeregrineV