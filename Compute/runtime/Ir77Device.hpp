#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77Compute.hpp"
#include "../dictionary/IDIr77Compute.hpp"

#include "../../Ir77RT/types/Ir77Types.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

#include "../interface/IIr77Device.hpp"

#include "../services/Ir77OpenCL.hpp"

#include "../types/Ir77Types.hpp"

using namespace NSIr77RT;

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77Device -- Compute's OpenCL device (Ir77OpenCL does the API work): found on Open, the programs run on it. One user at a time.
// =============================================================================================================================================
class Ir77Device : public Ir77Enlisted, public IIr77Device, public std::enable_shared_from_this<Ir77Device> {
   public:
    Ir77Device() {
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
        seat_shared_uuid<&GUIDIr77Device>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Compute>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Device)
            obj = std::shared_ptr<IIr77Device>(shared_from_this(), static_cast<IIr77Device*>(this));

        else if (iid == GUIDIr77Device)
            obj = std::shared_ptr<Ir77Device>(shared_from_this(), static_cast<Ir77Device*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> Open() {
        m_error.clear();

        if (!m_opencl.Open(m_error)) return Ir77RETURN<Ir77NotConfigured>(this, "[WARN] Ir77Device: " + m_error + " -- Compute programs need one.");

        return Ir77RETURN<Ir77OperationSucceeded>(this, "Ir77Device: " + m_opencl.Name());
    }

    std::shared_ptr<IIr77Return const> GetDevice(bool& opened, std::string& name, std::string& error) const {
        opened = m_opencl.IsOpen();
        name = opened ? m_opencl.Name() : std::string{};
        error = m_error;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> ListDevices(std::vector<std::string>& devices) const {
        devices = Ir77OpenCL::List();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> RunProgram(Ir77ProgramJob const& job, std::atomic<bool> const& cancel, Ir77ProgramResult& result, std::string& error) {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (!m_opencl.Run(job, cancel, result, error)) return Ir77RETURN<Ir77OperationFailed>(this, "Ir77Device: " + error);

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    Ir77OpenCL m_opencl{};

    std::mutex m_mutex{};  // one OpenCL user at a time

    std::string m_error{};  // why the last Open didn't find or build -- '' when it did
};
}  // namespace NSIr77Compute
