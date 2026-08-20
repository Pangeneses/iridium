#pragma once

#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIIr77PeregrineV.hpp"
#include "../dictionary/IDIr77PeregrineV.hpp"

#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"

#include "../interface/IIr77PVContext.hpp"

using namespace NSIr77RT;

namespace NSIr77PeregrineV {

class Ir77PVContext : public Ir77Enlisted, public IIr77PVContext, public std::enable_shared_from_this<Ir77PVContext> {
   public:
    Ir77PVContext() {
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
        seat_shared_uuid<&GUIDIr77PVContext>(uid);

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

        else if (iid == &GUIDIIr77PVContext)
            obj = std::shared_ptr<IIr77PVContext>(shared_from_this(), static_cast<IIr77PVContext*>(this));

        else if (iid == &GUIDIr77PVContext)
            obj = std::shared_ptr<Ir77PVContext>(shared_from_this(), static_cast<Ir77PVContext*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

   public:
    std::shared_ptr<IIr77Return const> BuildContext();

    std::shared_ptr<IIr77Return const> InitializePipeline();

    std::shared_ptr<IIr77Return const> CurrentDevice(std::uint32_t& index);

    std::shared_ptr<IIr77Return const> GetWindow(Ir77Window& window);

    std::shared_ptr<IIr77Return const> GetContext(std::map<std::uint64_t const, std::shared_ptr<IIr77Enlisted>>& context);

    std::shared_ptr<IIr77Return const> GetMemberByID(std::uint64_t const& id, std::shared_ptr<IIr77Enlisted>& context);

   private:
    Ir77Window m_window;

    std::map<const std::uint64_t, std::shared_ptr<IIr77Enlisted>> m_context;

    std::uint32_t m_index{0};
};
}  // namespace NSIr77PeregrineV