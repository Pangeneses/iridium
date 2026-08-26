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
#include "../../Ir77RT/runtime/Ir77Operand.hpp"

#include "../interface/IIr77PeregrineV.hpp"

#include "../interface/IIr77PVInstance.hpp"
#include "../interface/IIr77PVDevice.hpp"
#include "../interface/IIr77PVSwapchain.hpp"
#include "../interface/IIr77PVLayout.hpp"
#include "../interface/IIr77PVRenderPass.hpp"
#include "../interface/IIr77PVPipeline.hpp"
#include "../interface/IIr77PVCmdBuffer.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVLifetime : public Ir77Enlisted, public Ir77Operand, public IIr77PeregrineV, public std::enable_shared_from_this<Ir77PVLifetime> {
   public:
    Ir77PVLifetime() {
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

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77PeregrineV>(uid);

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

        else if (iid == &GUIDIIr77Operand)
            obj = std::shared_ptr<IIr77Operand>(shared_from_this(), static_cast<IIr77Operand*>(this));

        else if (iid == &GUIDIIr77PeregrineV)
            obj = std::shared_ptr<IIr77PeregrineV>(shared_from_this(), static_cast<IIr77PeregrineV*>(this));

        else if (iid == &GUIDIr77PeregrineV)
            obj = std::shared_ptr<Ir77PVLifetime>(shared_from_this(), static_cast<Ir77PVLifetime*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) {
        m_operand.at(at) = obj;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   public:
    static std::shared_ptr<IIr77Return const> CreateDeviceInterface(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs);

    static std::shared_ptr<IIr77Return const> CreateSwapchains(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs);
     /*
    static std::shared_ptr<IIr77Return const> Assign(std::shared_ptr<IIr77Operand const> lhs, std::shared_ptr<IIr77Operand const> rhs) {
   
        std::shared_ptr<IIr77Enlisted const> enlisted_rhs;
        rhs->GetIndexed(0, enlisted_rhs);

        auto rhs_mutable = std::const_pointer_cast<IIr77Operand>(rhs);

        auto raw = QueryAs<IIr77MPVMOP<Ir77String>>(&GUIDOPIr77String, std::const_pointer_cast<IIr77Enlisted>(enlisted_rhs).get());

        if (!raw.get()) return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid RHS Operand.");

        try {
            IsValidHEX(raw->Get());
        } catch (std::domain_error) {
            return Ir77RETURN<Ir77OperationFailed>(nullptr, "Invalid Operand.");
        }

        auto lhs_mutable = std::const_pointer_cast<IIr77Operand>(lhs);

        lhs_mutable->SetIndexed(0, enlisted_rhs);


        return Ir77RETURN<Ir77OperationSucceeded>();
    }
        */
   public:
    std::shared_ptr<IIr77Return const> CreateInstance();

    std::shared_ptr<IIr77Return const> EnumeratePhysicalDevices();

    std::shared_ptr<IIr77Return const> CreateSurfaces();

    std::shared_ptr<IIr77Return const> EnumerateDeviceQueues();

    std::shared_ptr<IIr77Return const> CreateLogicalDevices();

    std::shared_ptr<IIr77Return const> CreateLayout();

    std::shared_ptr<IIr77Return const> CreateRenderPass();

    std::shared_ptr<IIr77Return const> CreateSwapchains();

    std::shared_ptr<IIr77Return const> CreatePipelineGFX();

    std::shared_ptr<IIr77Return const> CreateCommandBuffers();

    std::shared_ptr<IIr77Return const> CreateShaders();

   private:
    std::uint32_t m_device_count;

    std::uint64_t m_current_device;

    std::shared_ptr<IIr77PVInstance> m_instance;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVDevice>> m_devices;

    std::map<std::uint64_t, std::vector<SDL_Window*>> m_windows;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVSwapchain>>> m_swapchains;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVLayout>> m_pipeline_layouts;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVRenderPass>> m_render_pass;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVPipeline>>> m_pipelines;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77PVCmdBuffer>>> m_command_buffers;

    std::map<std::uint64_t, std::shared_ptr<IIr77PVShader>> m_shaders;

    std::map<std::uint64_t, std::vector<std::shared_ptr<IIr77Enlisted>>> m_compute;
};
}  // namespace NSIr77PeregrineV