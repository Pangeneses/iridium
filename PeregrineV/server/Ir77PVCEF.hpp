#pragma once

#include <SDL3/SDL_video.h>

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "Ir77PeregrineV.hpp"

#include "../interface/IIr77PVOverlay.hpp"
#include "../runtime/Ir77PVOverlay.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVCEF : public Ir77Enlisted, public std::enable_shared_from_this<Ir77PVCEF> {
   public:
    Ir77PVCEF() {
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
        seat_shared_uuid<&GUIDIr77PVCEF>(uid);

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

        else if (iid == GUIDIr77PVCEF)
            obj = std::shared_ptr<Ir77PVCEF>(shared_from_this(), static_cast<Ir77PVCEF*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetPeregrineV(std::shared_ptr<Ir77PeregrineV>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // One overlay per window. Requires the context's CreateLayouts and CreateSwapchains;
    // call before Ir77PVPaint::CreateCommandBuffers so Paint can wire the overlays in.
    std::shared_ptr<IIr77Return const> CreateOverlays() {
        std::uint64_t const device_id = m_context->m_current_device;
        std::size_t const windows = m_context->m_windows.at(device_id).size();

        if (windows > MAX_WINDOWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCEF: too many windows.");

        std::shared_ptr<IIr77PVLayout> layout_cef{};
        if (m_context->GetLayout(Ir77PVLayoutKind::CEF, layout_cef)->ID() != GUIDIr77OperationSucceeded)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCEF: CEF layout not created -- call CreateLayouts first.");

        std::vector<std::shared_ptr<IIr77PVOverlay>> overlays{};

        for (std::size_t w = 0; w < windows; w++) {
            auto overlay = std::static_pointer_cast<IIr77PVOverlay>(std::make_shared<Ir77PVOverlay>());

            overlay->SetDevice(m_context->m_devices.at(device_id));

            overlay->SetAllocator(m_context->m_allocators.at(device_id));

            overlay->SetSwapchain(m_context->m_swapchains.at(device_id).at(w));

            overlay->SetLayout(layout_cef);

            if (overlay->CreateResources()->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCEF: overlay creation failed.");

            overlays.push_back(overlay);
        }

        m_context->m_overlays[device_id] = overlays;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Overlays.");
    }

    std::shared_ptr<IIr77Return const> GetOverlay(std::uint32_t const& window_index, std::shared_ptr<IIr77PVOverlay>& overlay) {
        auto const found = m_context->m_overlays.find(m_context->m_current_device);
        if (found == m_context->m_overlays.end() || window_index >= found->second.size())
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCEF: no overlay for window.");

        overlay = found->second[window_index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<Ir77PeregrineV> m_context{nullptr};
};
}  // namespace NSIr77PeregrineV