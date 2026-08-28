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

            cmd_buffer->SetPipeline(m_context->m_pipelines.at(m_context->m_current_device).at(i));

            cmd_buffer->DefineSyncObjects();

            cmd_buffer->DefineCommandPool();

            cmd_buffer->AllocateCommandBuffer();

            cmd_buffers.push_back(cmd_buffer);
        }

        m_context->m_command_buffers.emplace(m_context->m_current_device, cmd_buffers);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Command Buffers.");
    }

    std::shared_ptr<IIr77Return const> Next() {
        std::vector<std::shared_ptr<IIr77PVCmdBuffer>> cmd_buffers = m_context->m_command_buffers.at(m_context->m_current_device);
        for (int i = 0; i < cmd_buffers.size(); i++) {
            cmd_buffers[i]->WaitForFence();

            VkResult vk_result;
            cmd_buffers[i]->AcquireNextImage(&vk_result);

            if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR) {
                ResetSwapchain(i);
            } else if (vk_result != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: pipeline corupted.");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Next.");
    }

    std::shared_ptr<IIr77Return const> ResetSwapchain(std::uint32_t const& index) {
        std::vector<std::shared_ptr<IIr77PVSwapchain>> swapchains = m_context->m_swapchains.at(m_context->m_current_device);

        swapchains.at(index)->CleanupSwapchain();

        swapchains.at(index)->QuerySwapchainSupport();

        swapchains.at(index)->SwapSurfaceFormat();

        swapchains.at(index)->PresentMode();

        swapchains.at(index)->SurfaceCapabilities();

        swapchains.at(index)->InitSwapchainInfo();

        swapchains.at(index)->DefineSwapchain();

        swapchains.at(index)->InitSwapchainImages();

        swapchains.at(index)->DefineImageView();

        swapchains.at(index)->DefineFramebuffers();

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: ValidateSwapchain.");
    }

    std::shared_ptr<IIr77Return const> Draw() {
        std::vector<std::shared_ptr<IIr77PVCmdBuffer>> cmd_buffers = m_context->m_command_buffers.at(m_context->m_current_device);
        for (int i = 0; i < cmd_buffers.size(); i++) {
            cmd_buffers[i]->ResetFence();

            cmd_buffers[i]->RecordCommandBuffer();

            cmd_buffers[i]->SubmitFrame();

            VkResult vk_result;
            cmd_buffers[i]->PresentFrame(&vk_result);

            if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR) {
                ResetSwapchain(i);
            } else if (vk_result != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: pipeline corupted.");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Draw.");
    }

   private:
    std::shared_ptr<Ir77PeregrineV> m_context{nullptr};
};
}  // namespace NSIr77PeregrineV