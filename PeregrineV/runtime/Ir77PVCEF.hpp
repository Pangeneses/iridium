#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVSwapchain.hpp"
#include "../interface/IIr77PVDescriptorSet.hpp"
#include "../interface/IIr77PVOverlay.hpp"
#include "../interface/IIr77PVCEF.hpp"

#include "./Ir77PVOverlay.hpp"

#include "../types/Ir77PVTypes.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

// =============================================================================================================================================
// Ir77PVCEF -- the CEF overlays of one device: one Ir77PVOverlay per window, each with a texture slot in the bindless table.
//   Ir77PVInit wires it: SetDevice / SetAllocator / SetSwapchains / SetDescriptorSet -> CreateOverlays
// Owns the overlays; Paint gets them through Init (GetOverlays), CEF through PeregrineV (GetOverlay).
// =============================================================================================================================================
class Ir77PVCEF : public Ir77Enlisted, public IIr77PVCEF, public std::enable_shared_from_this<Ir77PVCEF> {
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
        seat_shared_uuid<&GUIDIIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77PVCEF)
            obj = std::shared_ptr<IIr77PVCEF>(shared_from_this(), static_cast<IIr77PVCEF*>(this));

        else if (iid == GUIDIr77PVCEF)
            obj = std::shared_ptr<Ir77PVCEF>(shared_from_this(), static_cast<Ir77PVCEF*>(this));

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

    std::shared_ptr<IIr77Return const> SetAllocator(VmaAllocator allocator) {
        m_allocator = allocator;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetSwapchains(std::vector<std::shared_ptr<IIr77PVSwapchain>> const& swapchains) {
        m_swapchains = swapchains;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetDescriptorSet(std::shared_ptr<IIr77PVDescriptorSet> descriptor_set) {
        m_descriptor_set = descriptor_set;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    // -------------------------------------------------------------------------------------------------------------------------------------
    // overlays
    // -------------------------------------------------------------------------------------------------------------------------------------

    // One overlay per window, each with a texture slot in the device's bindless table. Before Paint's command buffers, so Paint can wire
    // the overlays in.
    std::shared_ptr<IIr77Return const> CreateOverlays() {
        if (!m_device || m_allocator == VK_NULL_HANDLE || !m_descriptor_set)
            return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCEF: device, allocator or bindless table not set.");

        if (m_swapchains.size() > MAX_WINDOWS) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCEF: too many windows.");

        std::vector<std::shared_ptr<IIr77PVOverlay>> overlays{};

        for (auto const& swapchain : m_swapchains) {
            auto overlay = std::static_pointer_cast<IIr77PVOverlay>(std::make_shared<Ir77PVOverlay>());

            overlay->SetDevice(m_device);

            overlay->SetAllocator(m_allocator);

            overlay->SetSwapchain(swapchain);

            overlay->SetDescriptorSet(m_descriptor_set);

            if (overlay->CreateResources()->ID() != GUIDIr77OperationSucceeded)
                return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCEF: overlay creation failed.");

            overlays.push_back(overlay);
        }

        m_overlays = overlays;

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Overlays.");
    }

    std::shared_ptr<IIr77Return const> GetOverlay(std::uint32_t const& window_index, std::shared_ptr<IIr77PVOverlay>& overlay) {
        if (window_index >= m_overlays.size()) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77PVCEF: no overlay for window.");

        overlay = m_overlays[window_index];

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetOverlays(std::vector<std::shared_ptr<IIr77PVOverlay>>& overlays) {
        overlays = m_overlays;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<IIr77PVDevice> m_device{};

    VmaAllocator m_allocator{VK_NULL_HANDLE};

    std::vector<std::shared_ptr<IIr77PVSwapchain>> m_swapchains{};  // [window]

    std::shared_ptr<IIr77PVDescriptorSet> m_descriptor_set{};

    std::vector<std::shared_ptr<IIr77PVOverlay>> m_overlays{};  // [window]
};
}  // namespace NSIr77PeregrineV
