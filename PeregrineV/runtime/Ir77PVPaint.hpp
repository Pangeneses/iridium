#pragma once

#include <SDL3/SDL_video.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../types/Ir77PVTypes.hpp"

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77PVPaint.hpp"
#include "../interface/IIr77PVCmdBuffer.hpp"

#include "./Ir77PVCmdBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVPaint -- records and presents every window of one device: one Ir77PVCmdBuffer per window.
//   Ir77PVInit wires it: SetDevice / SetWindows / SetSwapchains / SetRenderPasses / SetLayout / SetDescriptorSet / SetMaterialTableSlot /
//   SetPipelineCache / SetShadowMap / SetFrameSlots / SetFrameBuffers / SetFrameTables / SetCommandSlots / SetOverlays / [SetAcceleration] ->
//   CreateCommandBuffers (after content is loaded)
//   per frame: SetFrameData / SetDraws / SetFrameDispatches / SetViewDispatches / SetPassList -> Draw
// The once-a-frame work (SetFrameDispatches, the acceleration build) goes to the first window Draw records -- window 0 unless it's minimized.
// The view dispatches (the cull pass) go to every window, once per view.
// Owns the command buffers, each window's frame-in-flight index and the resize flag.
// =============================================================================================================================================
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
        seat_shared_uuid<&GUIDIIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVPaint)
            obj = std::shared_ptr<IIr77PVPaint>(shared_from_this(), static_cast<IIr77PVPaint*>(this));

        else if (iid == GUIDIr77PVPaint)
            obj = std::shared_ptr<Ir77PVPaint>(shared_from_this(), static_cast<Ir77PVPaint*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // wiring -- Ir77PVInit
    // -------------------------------------------------------------------------------------------------------------------------------------
   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77PVDevice> device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetWindows(std::vector<SDL_Window*> const& windows) {
        m_windows = windows;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetSwapchains(std::vector<std::shared_ptr<IIr77PVSwapchain>> const& swapchains) {
        m_swapchains = swapchains;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetRenderPasses(std::map<Ir77PVRenderPassKind, std::shared_ptr<IIr77PVRenderPass>> const& render_passes) {
        m_render_passes = render_passes;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetLayout(std::shared_ptr<IIr77PVLayout> layout) {
        m_layout = layout;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) {
        m_descriptor_set = descriptor_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetMaterialTableSlot(std::uint32_t const& slot) {
        m_material_table = slot;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetPipelineCache(std::shared_ptr<IIr77PVPipelineCache> pipeline_cache) {
        m_pipeline_cache = pipeline_cache;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetShadowMap(std::shared_ptr<IIr77PVTexture> shadow_map) {
        m_shadow_map = shadow_map;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetFrameSlots(std::vector<std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT>> const& frame_slots) {
        m_frame_slots = frame_slots;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetFrameBuffers(std::vector<std::map<Ir77PVBufferSlot, std::shared_ptr<IIr77PVBuffer>>> const& frame_buffers) {
        m_frame_buffers = frame_buffers;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetOverlays(std::vector<std::shared_ptr<IIr77PVOverlay>> const& overlays) {
        m_overlays = overlays;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Ray-query devices -- the first window recorded each frame builds the acceleration structures after its Compute pass
    std::shared_ptr<IIr77Return const> SetAcceleration(std::shared_ptr<IIr77PVAcceleration> acceleration) {
        m_acceleration = acceleration;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // [window] -- the frame table buffers (one Dynamic copy per frame in flight). Paint rewrites each window's copy before submitting it, so
    // it names the TLAS built this frame (tlas); the other slots stay what SetFrameSlots says.
    std::shared_ptr<IIr77Return const> SetFrameTables(std::vector<std::shared_ptr<IIr77PVBuffer>> const& frame_tables) {
        m_frame_tables = frame_tables;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // [frame in flight] -- Geometry's command pool slot for each frame copy; every frame table names its frame's (what the cull pass reads)
    std::shared_ptr<IIr77Return const> SetCommandSlots(std::array<std::uint32_t, MAX_FRAMES_IN_FLIGHT> const& slots) {
        m_command_slots = slots;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // setup -- after the wiring and the pipeline cache. Overlays are optional (wired if there is one per window).
    // Every command buffer shares the device's bindless layout + table; each gets its window's frame slots.
    // -------------------------------------------------------------------------------------------------------------------------------------
    std::shared_ptr<IIr77Return const> CreateCommandBuffers() {
        std::size_t const windows = m_swapchains.size();

        if (windows == 0 || windows > MAX_WINDOWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: no swapchains, or too many windows.");

        if (m_windows.size() != windows || m_frame_slots.size() != windows || m_frame_buffers.size() != windows)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: windows, swapchains, frame slots and frame buffers must match one per window.");

        if (!m_device || !m_layout || !m_descriptor_set)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: device, bindless layout or table not set.");

        if (m_render_passes.count(Ir77PVRenderPassKind::Main) == 0)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: main render pass not set.");

        if (m_material_table == BINDLESS_NO_SLOT) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: material table slot not set.");

        if (!m_pipeline_cache) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: pipeline cache not set.");

        bool const has_overlays = m_overlays.size() == windows;

        // the CEF role's handle -- re-resolved every frame, so a manifest reload re-points the overlay too
        Ir77PVPipelineHandle pipeline_cef{IR77PV_NO_PIPELINE};
        m_pipeline_cache->FindPipeline(Ir77PVPipelineKind::CEF, pipeline_cef);

        std::vector<std::shared_ptr<IIr77PVCmdBuffer>> cmd_buffers{};

        for (std::size_t w = 0; w < windows; w++) {
            auto cmd_buffer = std::static_pointer_cast<IIr77PVCmdBuffer>(std::make_shared<Ir77PVCmdBuffer>());

            cmd_buffer->SetDevice(m_device);

            cmd_buffer->SetSwapchain(m_swapchains[w]);

            for (auto const& [kind, render_pass] : m_render_passes) cmd_buffer->SetRenderPass(kind, render_pass);

            cmd_buffer->SetLayout(m_layout);

            cmd_buffer->SetDescriptorSet(m_descriptor_set);

            cmd_buffer->SetMaterialTable(m_material_table);

            // the device's shadow map -- without it the Shadow pass has no target and is skipped
            if (m_shadow_map) {
                VkImage image{VK_NULL_HANDLE};
                VkImageView view{VK_NULL_HANDLE};
                VkExtent2D extent{};

                m_shadow_map->GetImage(&image);
                m_shadow_map->GetView(&view);
                m_shadow_map->GetExtent(&extent);

                cmd_buffer->SetShadowTarget(image, view, extent);
            }

            cmd_buffer->SetPipelineCache(m_pipeline_cache);

            cmd_buffer->SetFrameSlots(m_frame_slots[w]);

            if (has_overlays) cmd_buffer->SetOverlay(m_overlays[w], pipeline_cef);

            if (cmd_buffer->CreateCommandBuffer()->ID() != GUIDIr77OperationSucceeded) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: command buffer setup failed.");
            }

            cmd_buffers.push_back(cmd_buffer);
        }

        m_command_buffers = cmd_buffers;

        m_frames.assign(windows, 0);

        m_frame_data.assign(windows, {});

        m_builder = windows;  // none yet

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Command Buffers.");
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // per-frame input -- safe to call any time; applied inside Draw once the frame's fence has signalled
    // -------------------------------------------------------------------------------------------------------------------------------------

    // Latest contents for a frame-level buffer (camera, lights, instances ...). Re-applied every frame so every
    // frame-in-flight copy stays current; call again only when the data changes.
    std::shared_ptr<IIr77Return const> SetFrameData(std::size_t const& window, Ir77PVBufferSlot const& slot, void const* data, VkDeviceSize const& size,
                                                    VkDeviceSize const& offset = 0) {
        if (window >= m_frame_data.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: window index out of range.");
        if (data == nullptr || size == 0) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: empty frame data.");

        Ir77PVFrameWrite write{};
        write.offset = offset;
        write.bytes.resize(static_cast<std::size_t>(size));
        std::memcpy(write.bytes.data(), data, static_cast<std::size_t>(size));

        m_frame_data[window][slot] = std::move(write);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Replaces one view's draw list for a pass in a window. Persists until replaced.
    std::shared_ptr<IIr77Return const> SetDraws(std::size_t const& window, std::uint32_t const& view, Ir77PVPass const& pass,
                                                std::vector<Ir77PVDrawItem> const& draws) {
        if (window >= m_command_buffers.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: window index out of range.");

        return m_command_buffers[window]->SetDraws(view, pass, draws);
    }

    // Replaces the once-a-frame compute list (skinning, the field) -- recorded by whichever window Paint records first each frame, ahead of
    // the acceleration build. Persists until replaced.
    std::shared_ptr<IIr77Return const> SetFrameDispatches(std::vector<Ir77PVDispatch> const& dispatches) {
        m_frame_dispatches = dispatches;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Replaces the per-view compute list (the cull pass) -- every window records it once per view it draws, after the once-a-frame work.
    // Persists until replaced.
    std::shared_ptr<IIr77Return const> SetViewDispatches(std::vector<Ir77PVDispatch> const& dispatches) {
        m_view_dispatches = dispatches;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // Replaces a window's pass list -- what its command buffer records each frame, in order. At least one pass must target the swapchain.
    std::shared_ptr<IIr77Return const> SetPassList(std::size_t const& window, std::vector<Ir77PVPassDesc> const& passes) {
        if (window >= m_command_buffers.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: window index out of range.");

        return m_command_buffers[window]->SetPassList(passes);
    }

    std::shared_ptr<IIr77Return const> ClearDraws(std::size_t const& window) {
        if (window >= m_command_buffers.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: window index out of range.");

        return m_command_buffers[window]->ClearDraws();
    }

    /********************************************* DRAW ********************************************************/
    std::shared_ptr<IIr77Return const> Draw() {
        if (m_resize_in_progress) return Ir77RETURN<Ir77OperationSucceeded>();

        auto const now = std::chrono::steady_clock::now();
        m_delta_time = std::chrono::duration<float>(now - m_last_frame_time).count();
        m_last_frame_time = now;
        m_fps = m_delta_time > 0.0f ? 1.0f / m_delta_time : 0.0f;

        auto& frames = m_frames;

        // the builder -- the first window recorded this frame runs the once-a-frame work (frame dispatches, acceleration build). When it moves
        // (the old one minimized), the old builder's frames are waited for first: they may still be reading what the new one is about to
        // rewrite (the instance array, the shared scratch).
        std::size_t builder{m_command_buffers.size()};
        for (std::size_t i = 0; i < m_command_buffers.size() && builder == m_command_buffers.size(); i++)
            if (!(SDL_GetWindowFlags(m_windows.at(i)) & SDL_WINDOW_MINIMIZED)) builder = i;

        if (builder != m_builder && m_builder < m_command_buffers.size()) m_command_buffers[m_builder]->WaitAllFrames();
        m_builder = builder;

        for (std::size_t i = 0; i < m_command_buffers.size(); i++) {
            if (SDL_GetWindowFlags(m_windows.at(i)) & SDL_WINDOW_MINIMIZED) continue;

            auto const& cmd_buffer = m_command_buffers[i];

            cmd_buffer->SetCurrentFrame(frames.at(i));

            // the builder gets the frame's dispatches and the acceleration build; the rest have no compute
            std::vector<Ir77PVDispatch> const dispatches = i == builder ? m_frame_dispatches : std::vector<Ir77PVDispatch>{};

            cmd_buffer->SetDispatches(dispatches);
            cmd_buffer->SetViewDispatches(m_view_dispatches);
            cmd_buffer->SetAcceleration(i == builder ? m_acceleration : std::shared_ptr<IIr77PVAcceleration>{});

            if (cmd_buffer->WaitForFence()->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: fence wait failed.");

            ApplyFrameData(i, frames.at(i));

            VkResult acquire_result{VK_SUCCESS};
            cmd_buffer->AcquireNextImage(&acquire_result);

            if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
                if (ResetSwapchain(static_cast<std::uint32_t>(i))->ID() != GUIDIr77OperationSucceeded)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: swapchain rebuild failed.");

                cmd_buffer->AcquireNextImage(&acquire_result);

                if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR)
                    return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: acquire failed after rebuild.");
            } else if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: acquire failed.");
            }

            cmd_buffer->ResetFence();

            if (cmd_buffer->RecordCommandBuffer()->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: record failed.");

            // the TLAS this frame traces -- the builder's record just built it. Written into this window's frame table copy before submit; the
            // copy is free (its fence was waited above).
            DefineFrameTable(i, frames.at(i));

            if (cmd_buffer->SubmitFrame()->ID() != GUIDIr77OperationSucceeded) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: submit failed.");

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
        // flags are cleared on every exit path -- a failed step used to leave Draw disabled forever
        std::shared_ptr<IIr77PVOverlay> overlay{};
        if (index < m_overlays.size()) overlay = m_overlays[index];

        m_resize_in_progress = true;
        if (overlay) overlay->SetResizing(true);

        auto const result = RebuildSwapchain(index);

        if (overlay) overlay->SetResizing(false);
        m_resize_in_progress = false;

        return result;
    }

   private:
    // Pending write for one frame-level buffer slot
    struct Ir77PVFrameWrite {
        std::vector<std::uint8_t> bytes{};

        VkDeviceSize offset{0};
    };

    // This window's frame tables for this frame, one per view, with the latest TLAS slot and this frame's command pool -- everything else as
    // registered
    void DefineFrameTable(std::size_t const& window, std::uint32_t const& frame) {
        if (window >= m_frame_tables.size() || window >= m_frame_slots.size() || !m_frame_tables[window]) return;

        Ir77PVFrameSlots const& slots = m_frame_slots[window][frame];
        Ir77PVFrameTable values = slots.values;

        values.tlas = BINDLESS_NO_SLOT;
        values.ray_objects = BINDLESS_NO_SLOT;
        values.commands = m_command_slots[frame];

        if (m_acceleration) {
            m_acceleration->GetTopLevelSlot(values.tlas);
            m_acceleration->GetObjectSlot(values.ray_objects);
        }

        for (std::uint32_t view = 0; view < MAX_VIEWS; view++) {
            values.camera = slots.cameras[view];

            m_frame_tables[window]->Update(frame, &values, sizeof(Ir77PVFrameTable), view * VIEW_STRIDE);
        }
    }

    void ApplyFrameData(std::size_t const& window, std::uint32_t const& frame) {
        if (window >= m_frame_data.size() || window >= m_frame_buffers.size()) return;

        for (auto const& [slot, write] : m_frame_data[window]) {
            auto const found = m_frame_buffers[window].find(slot);
            if (found == m_frame_buffers[window].end() || !found->second) continue;

            found->second->Update(frame, write.bytes.data(), static_cast<VkDeviceSize>(write.bytes.size()), write.offset);
        }
    }

    std::shared_ptr<IIr77Return const> RebuildSwapchain(std::uint32_t const& index) {
        if (index >= m_command_buffers.size() || index >= m_swapchains.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVPaint: window index out of range.");

        m_command_buffers[index]->WaitAllFrames();

        auto const& swapchain = m_swapchains[index];

        auto const result = swapchain->RebuildSwapchain();
        if (result->ID() != GUIDIr77OperationSucceeded) return result;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Reset Swapchain.");
    }

   private:
    // wiring -- Ir77PVInit
    std::shared_ptr<IIr77PVDevice> m_device{};

    std::vector<SDL_Window*> m_windows{};  // [window]

    std::vector<std::shared_ptr<IIr77PVSwapchain>> m_swapchains{};  // [window]

    std::map<Ir77PVRenderPassKind, std::shared_ptr<IIr77PVRenderPass>> m_render_passes{};

    std::shared_ptr<IIr77PVLayout> m_layout{};

    std::shared_ptr<IIr77PVDescriptorSet> m_descriptor_set{};

    std::uint32_t m_material_table{BINDLESS_NO_SLOT};

    std::shared_ptr<IIr77PVPipelineCache> m_pipeline_cache{};

    std::shared_ptr<IIr77PVTexture> m_shadow_map{};

    std::vector<std::array<Ir77PVFrameSlots, MAX_FRAMES_IN_FLIGHT>> m_frame_slots{};  // [window][frame]

    std::vector<std::map<Ir77PVBufferSlot, std::shared_ptr<IIr77PVBuffer>>> m_frame_buffers{};  // [window][slot]

    std::vector<std::shared_ptr<IIr77PVOverlay>> m_overlays{};  // [window] -- empty = no overlay

    std::shared_ptr<IIr77PVAcceleration> m_acceleration{};  // ray-query devices -- handed to the frame's first recorded window

    std::vector<std::shared_ptr<IIr77PVBuffer>> m_frame_tables{};  // [window] -- Dynamic, one copy per frame in flight

    // owned
    std::vector<std::shared_ptr<IIr77PVCmdBuffer>> m_command_buffers{};  // [window]

    std::vector<std::uint32_t> m_frames{};  // [window] -- frame in flight each command buffer records next

    std::vector<Ir77PVDispatch> m_frame_dispatches{};  // once a frame -- the builder records them

    std::vector<Ir77PVDispatch> m_view_dispatches{};  // every window, once per view -- the cull pass

    std::array<std::uint32_t, MAX_FRAMES_IN_FLIGHT> m_command_slots{BINDLESS_NO_SLOT, BINDLESS_NO_SLOT, BINDLESS_NO_SLOT};  // Geometry's command pool

    std::size_t m_builder{0};  // the window that ran the once-a-frame work last Draw (window count = none yet)

    bool m_resize_in_progress{false};

    // [window][slot]
    std::vector<std::map<Ir77PVBufferSlot, Ir77PVFrameWrite>> m_frame_data{};

    std::chrono::steady_clock::time_point m_last_frame_time{std::chrono::steady_clock::now()};

    float m_delta_time{0.0f};

    float m_fps{0.0f};
};
}  // namespace NSIr77PeregrineV