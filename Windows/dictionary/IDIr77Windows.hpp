#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../Ir77RT/interface/IIr77GUID.hpp"
#include "../../Ir77RT/interface/IIr77Dictionary.hpp"
#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Enlisted.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77CEF {

inline Ir77GUID GUIDIr77Windows{(static_cast<unsigned __int128>(0x28FFC68D056C4971) << 64) | 0xA3EBE7E6593290A6};

class IDIr77Windows : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIr77Windows> {
   public:
    IDIr77Windows() {
        m_enlisted = std::chrono::system_clock::now();

        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }
    }

    // ── IIr77Enlisted ────────────────────────────────────────────
   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Dictionary>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIr77Windows>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIr77Windows>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == GUIDIr77Windows)
            obj = std::shared_ptr<IDIr77Windows>(shared_from_this(), static_cast<IDIr77Windows*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIr77Windows)); }

    IIr77GUID const* Member(std::string const& member) const { return m_index.at(member); }

    std::map<std::string, IIr77GUID const*> const& Index() const { return m_index; }

    // ── Data ─────────────────────────────────────────────────────
   private:
    Ir77GUID m_enlisted_uuid{};

    std::chrono::system_clock::time_point m_enlisted{};

    std::chrono::system_clock::time_point m_delisted{};

    std::shared_ptr<IIr77Enlisted const> m_sender{nullptr};

    std::string m_message{""};

    bool m_valid{true};

    std::shared_ptr<IIr77Return const> m_invalidation_condition{nullptr};

    std::map<std::string, IIr77GUID const*> m_index{
        {"GUIDIr77Windows", &GUIDIr77Windows},
    };
};

}  // namespace NSIr77Scene