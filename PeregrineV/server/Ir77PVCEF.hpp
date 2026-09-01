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

#include "IIr77PVCEF.hpp"

#include "Ir77PeregrineV.hpp"

#include "../runtime/Buffer/Ir77PVBufferCEF.hpp"
#include "../runtime/Pipeline Layout/Ir77PVLayoutCEF.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

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
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77PVCEF)
            obj = std::shared_ptr<IIr77PVCEF>(shared_from_this(), static_cast<IIr77PVCEF*>(this));

        else if (iid == &GUIDIr77PVCEF)
            obj = std::shared_ptr<Ir77PVCEF>(shared_from_this(), static_cast<Ir77PVCEF*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetPeregrineV(std::shared_ptr<Ir77PeregrineV>& context) {
        m_context = context;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CreateLayoutCEF() {
        auto layout_cef = std::static_pointer_cast<IIr77PVLayout>(std::make_shared<Ir77PVLayoutCEF>());

        layout_cef->SetDevice(m_context->m_devices.at(m_context->m_current_device));

        layout_cef->DefineDescriptorSetLayout();

        layout_cef->DefineDescriptorPool(8);

        layout_cef->DefinePipelineLayout();

        m_context->m_layouts_cef.emplace(m_context->m_current_device, layout_cef);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Layout CEF.");
    }

    std::shared_ptr<IIr77Return const> CreateBufferCEF() {
        std::vector<SDL_Window*> windows = m_context->m_windows.at(m_context->m_current_device);

        if (windows.size() > 8) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Vulkan: too many windows.");

        std::vector<std::shared_ptr<Ir77PVBufferCEF>> cef_buffers;
        for (int i = 0; i < windows.size(); i++) {
            auto cef_buffer = std::make_shared<Ir77PVBufferCEF>();

            cef_buffer->SetDevice(m_context->m_devices.at(m_context->m_current_device));

            cef_buffer->SetAllocator(m_context->m_allocators.at(m_context->m_current_device));

            cef_buffer->SetSwapchain(m_context->m_swapchains.at(m_context->m_current_device).at(i));

            cef_buffer->SetLayoutCEF(m_context->m_layouts_cef.at(m_context->m_current_device));

            cef_buffer->CreateResources();

            cef_buffers.push_back(cef_buffer);
        }

        m_context->m_buffer_cef.emplace(m_context->m_current_device, cef_buffers);

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Success: Create Buffer CEF.");
    }

    std::shared_ptr<IIr77Return const> GetBufferCEF(std::uint32_t window_index, std::shared_ptr<Ir77PVBufferCEF>& buffer) {
        buffer = m_context->m_buffer_cef.at(m_context->m_current_device).at(window_index);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    std::shared_ptr<Ir77PeregrineV> m_context{nullptr};
};
}  // namespace NSIr77PeregrineV