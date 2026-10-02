#pragma once

#include <SDL3/SDL_video.h>

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../server/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "Ir77PeregrineV.hpp"

#include "../runtime/Ir77PVCmdBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVPaint : public Ir77Enlisted, public std::enable_shared_from_this<Ir77PVPaint> {
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

    // -------------------------------------------------------------------------------------------------------------------------------------
    // setup -- after CreateDescriptorSets and CreatePipelines. Overlays are optional (wired if Ir77PVCEF created them).
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateCommandBuffers() {
        std::uint64_t const device_id = m_context->m_current_device;
        std::size_t const windows = m_context->m_windows.at(device_id).size();

        if (windows > Ir77PeregrineV::MAX_WINDOWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: too many windows.");

        auto const overlays = m_context->m_overlays.find(device_id);
        bool const has_overlays = overlays != m_context->m_overlays.end() && overlays->second.size() == windows;

        std::vector<std::shared_ptr<IIr77PVCmdBuffer>> cmd_buffers{};

        auto const passes = m_context->m_render_passes.find(device_id);
        if (passes == m_context->m_render_passes.end() || passes->second.count(Ir77PVRenderPassKind::Main) == 0)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: main render pass must be created before command buffers.");

        for (std::size_t w = 0; w < windows; w++) {
            auto cmd_buffer = std::static_pointer_cast<IIr77PVCmdBuffer>(std::make_shared<Ir77PVCmdBuffer>());

            cmd_buffer->SetInstance(m_context->m_instance);

            cmd_buffer->SetDevice(m_context->m_devices.at(device_id));

            cmd_buffer->SetSwapchain(m_context->m_swapchains.at(device_id).at(w));

            for (auto const& [kind, render_pass] : passes->second) cmd_buffer->SetRenderPass(kind, render_pass);

            cmd_buffer->SetGlobalSet(m_context->m_descriptor_sets_global.at(device_id).at(w));

            cmd_buffer->SetPassSet(m_context->m_descriptor_sets_pass.at(device_id).at(w));

            if (has_overlays) {
                std::shared_ptr<IIr77PVPipeline> pipeline_cef{};
                std::shared_ptr<IIr77PVLayout> layout_cef{};

                if (m_context->GetPipeline(w, Ir77PVPipelineKind::CEF, pipeline_cef)->ID() == &GUIDIr77OperationSucceeded &&
                    m_context->GetLayout(Ir77PVLayoutKind::CEF, layout_cef)->ID() == &GUIDIr77OperationSucceeded) {
                    cmd_buffer->SetOverlay(overlays->second.at(w), pipeline_cef, layout_cef);
                }
            }

            if (cmd_buffer->DefineSyncObjects()->ID() != &GUIDIr77OperationSucceeded || cmd_buffer->DefineCommandPool()->ID() != &GUIDIr77OperationSucceeded ||
                cmd_buffer->AllocateCommandBuffer()->ID() != &GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: command buffer setup failed.");
            }

            cmd_buffers.push_back(cmd_buffer);
        }

        m_context->m_command_buffers[device_id] = cmd_buffers;

        m_frame_data[device_id].assign(windows, {});

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Command Buffers.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // per-frame input -- safe to call any time; applied inside Draw once the frame's fence has signalled
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Latest contents for a frame-level buffer (camera, lights, instances ...). Re-applied every frame so every
    // frame-in-flight copy stays current; call again only when the data changes.
    std::shared_ptr<IIr77Return const> SetFrameData(std::size_t const& window, Ir77PVBufferSlot const& slot, void const* data, VkDeviceSize const& size,
                                                    VkDeviceSize const& offset = 0) {
        auto& windows = m_frame_data[m_context->m_current_device];
        if (window >= windows.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: window index out of range.");
        if (data == nullptr || size == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: empty frame data.");

        Ir77PVFrameWrite write{};
        write.offset = offset;
        write.bytes.resize(static_cast<std::size_t>(size));
        std::memcpy(write.bytes.data(), data, static_cast<std::size_t>(size));

        windows[window][slot] = std::move(write);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Replaces one pass's draw list for a window. Persists until replaced.
    std::shared_ptr<IIr77Return const> SetDraws(std::size_t const& window, Ir77PVPass const& pass, std::vector<Ir77PVDrawItem> const& draws) {
        auto const found = m_context->m_command_buffers.find(m_context->m_current_device);
        if (found == m_context->m_command_buffers.end() || window >= found->second.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: window index out of range.");

        return found->second[window]->SetDraws(pass, draws);
    }

    std::shared_ptr<IIr77Return const> ClearDraws(std::size_t const& window) {
        auto const found = m_context->m_command_buffers.find(m_context->m_current_device);
        if (found == m_context->m_command_buffers.end() || window >= found->second.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: window index out of range.");

        return found->second[window]->ClearDraws();
    }

    /********************************************* DRAW ********************************************************/
    std::shared_ptr<IIr77Return const> Draw() {
        if (m_context->m_resize_in_progress) return Ir77RETURN<Ir77OperationSucceeded>();

        auto const now = std::chrono::steady_clock::now();
        m_delta_time = std::chrono::duration<float>(now - m_last_frame_time).count();
        m_last_frame_time = now;
        m_fps = m_delta_time > 0.0f ? 1.0f / m_delta_time : 0.0f;

        std::uint64_t const device_id = m_context->m_current_device;

        auto const& cmd_buffers = m_context->m_command_buffers.at(device_id);
        auto& frames = m_context->m_current_frames.at(device_id);
        auto const& windows = m_context->m_windows.at(device_id);

        for (std::size_t i = 0; i < cmd_buffers.size(); i++) {
            if (SDL_GetWindowFlags(windows.at(i)) & SDL_WINDOW_MINIMIZED) continue;

            auto const& cmd_buffer = cmd_buffers[i];

            cmd_buffer->SetCurrentFrame(frames.at(i));

            if (cmd_buffer->WaitForFence()->ID() != &GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: fence wait failed.");

            ApplyFrameData(i, frames.at(i));

            VkResult acquire_result{VK_SUCCESS};
            cmd_buffer->AcquireNextImage(&acquire_result);

            if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
                if (ResetSwapchain(static_cast<std::uint32_t>(i))->ID() != &GUIDIr77OperationSucceeded)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: swapchain rebuild failed.");

                cmd_buffer->AcquireNextImage(&acquire_result);

                if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: acquire failed after rebuild.");
            } else if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: acquire failed.");
            }

            cmd_buffer->ResetFence();

            if (cmd_buffer->RecordCommandBuffer()->ID() != &GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: record failed.");

            if (cmd_buffer->SubmitFrame()->ID() != &GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: submit failed.");

            VkResult present_result{VK_SUCCESS};
            cmd_buffer->PresentFrame(&present_result);

            frames.at(i) = (frames.at(i) + 1) % MAX_FRAMES_IN_FLIGHT;

            if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR || acquire_result == VK_SUBOPTIMAL_KHR) {
                ResetSwapchain(static_cast<std::uint32_t>(i));
            } else if (present_result != VK_SUCCESS) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: present failed.");
            }
        }

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> ResetSwapchain(std::uint32_t const& index) {
        std::uint64_t const device_id = m_context->m_current_device;

        // flags are cleared on every exit path -- a failed step used to leave Draw disabled forever
        std::shared_ptr<IIr77PVOverlay> overlay{};
        auto const overlays = m_context->m_overlays.find(device_id);
        if (overlays != m_context->m_overlays.end() && index < overlays->second.size()) overlay = overlays->second[index];

        m_context->m_resize_in_progress = true;
        if (overlay) overlay->SetResizing(true);

        auto const result = RebuildSwapchain(index);

        if (overlay) overlay->SetResizing(false);
        m_context->m_resize_in_progress = false;

        return result;
    }

   private:
    // Pending write for one frame-level buffer slot
    struct Ir77PVFrameWrite {
        std::vector<std::uint8_t> bytes{};

        VkDeviceSize offset{0};
    };

    void ApplyFrameData(std::size_t const& window, std::uint32_t const& frame) {
        auto const device = m_frame_data.find(m_context->m_current_device);
        if (device == m_frame_data.end() || window >= device->second.size()) return;

        for (auto const& [slot, write] : device->second[window]) {
            std::shared_ptr<IIr77PVBuffer> buffer{};
            if (m_context->GetFrameBuffer(window, slot, buffer)->ID() != &GUIDIr77OperationSucceeded || !buffer) continue;

            buffer->Update(frame, write.bytes.data(), static_cast<VkDeviceSize>(write.bytes.size()), write.offset);
        }
    }

    std::shared_ptr<IIr77Return const> RebuildSwapchain(std::uint32_t const& index) {
        m_context->m_command_buffers.at(m_context->m_current_device).at(index)->WaitAllFrames();

        auto const& swapchain = m_context->m_swapchains.at(m_context->m_current_device).at(index);

        if (swapchain->QuerySwapchainSupport()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: QuerySwapchainSupport failed.");

        if (swapchain->SwapSurfaceFormat()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SwapSurfaceFormat failed.");

        if (swapchain->PresentMode()->ID() != &GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: PresentMode failed.");

        if (swapchain->SurfaceCapabilities()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: SurfaceCapabilities failed.");

        if (swapchain->DefineSwapchain()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: DefineSwapchain failed.");

        if (swapchain->CleanupSwapchain()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: CleanupSwapchain failed.");

        if (swapchain->InitSwapchainImages()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: InitSwapchainImages failed.");

        if (swapchain->DefineImageView()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: DefineImageView failed.");

        if (swapchain->DefineFramebuffers()->ID() != &GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVSwapchain: DefineFramebuffers failed.");

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Reset Swapchain.");
    }

   private:
    std::shared_ptr<Ir77PeregrineV> m_context{nullptr};

    // [device][window][slot]
    std::map<std::uint64_t, std::vector<std::map<Ir77PVBufferSlot, Ir77PVFrameWrite>>> m_frame_data{};

    std::chrono::steady_clock::time_point m_last_frame_time{std::chrono::steady_clock::now()};

    float m_delta_time{0.0f};

    float m_fps{0.0f};
};
}  // namespace NSIr77PeregrineV