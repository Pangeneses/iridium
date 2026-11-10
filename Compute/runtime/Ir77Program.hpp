#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
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
#include "../interface/IIr77Program.hpp"

#include "../types/Ir77Types.hpp"

using namespace NSIr77RT;

namespace NSIr77Compute {

// =============================================================================================================================================
// Ir77Program -- runs a Compute program on the device, on one worker thread so the frame loop never waits on it. The worker writes the
// result under the lock and flips the state last; the main thread reads the state, then the result. Destroyed mid-run: cancelled and joined.
// =============================================================================================================================================
class Ir77Program : public Ir77Enlisted, public IIr77Program, public std::enable_shared_from_this<Ir77Program> {
   public:
    Ir77Program() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    ~Ir77Program() {
        m_cancel = true;
        if (m_worker.joinable()) m_worker.join();
    }

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77Program>(uid);

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

        else if (iid == GUIDIIr77Program)
            obj = std::shared_ptr<IIr77Program>(shared_from_this(), static_cast<IIr77Program*>(this));

        else if (iid == GUIDIr77Program)
            obj = std::shared_ptr<Ir77Program>(shared_from_this(), static_cast<Ir77Program*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> SetDevice(std::shared_ptr<IIr77Device> const& device) {
        m_device = device;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Start(Ir77ProgramJob const& job) {
        if (m_state == Ir77JobState::Running) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Program: a run is already going.");
        if (!m_device) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Program: SetDevice not called.");

        if (m_worker.joinable()) m_worker.join();

        m_cancel = false;
        m_state = Ir77JobState::Running;

        m_worker = std::thread([this, job] { Work(job); });

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Cancel() {
        m_cancel = true;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetState(Ir77JobState& state) const {
        state = m_state;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> TakeResult(Ir77ProgramJob& job, Ir77ProgramResult& result, std::string& error) {
        Ir77JobState const state = m_state;
        if (state == Ir77JobState::Running || state == Ir77JobState::Idle) return Ir77RETURN<Ir77NotConfigured>(this, "Ir77Program: no finished run.");

        std::lock_guard<std::mutex> lock{m_mutex};
        job = std::move(m_job);
        result = std::move(m_result);
        error = std::move(m_error);
        m_job = Ir77ProgramJob{};
        m_result = Ir77ProgramResult{};
        m_error.clear();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

   private:
    void Work(Ir77ProgramJob const job) {
        Ir77ProgramResult result{};
        std::string error{};
        bool const ran = m_device->RunProgram(job, m_cancel, result, error)->ID() == GUIDIr77OperationSucceeded;

        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_job = job;
            m_result = std::move(result);
            m_error = error;
        }

        m_state = ran ? Ir77JobState::Done : m_cancel ? Ir77JobState::Cancelled : Ir77JobState::Failed;
    }

    std::shared_ptr<IIr77Device> m_device{};

    std::thread m_worker{};

    std::atomic<bool> m_cancel{false};

    std::atomic<Ir77JobState> m_state{Ir77JobState::Idle};

    mutable std::mutex m_mutex{};  // the result below -- the worker writes it, the main thread reads it

    Ir77ProgramJob m_job{};

    Ir77ProgramResult m_result{};

    std::string m_error{};
};
}  // namespace NSIr77Compute
