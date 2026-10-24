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

inline Ir77GUID GUIDIIr77Windows{(static_cast<unsigned __int128>(0xEED370DD01EA4E87) << 64) | 0xB6F49BE6F1A9031B};

inline Ir77GUID GUIDIIr77Server{(static_cast<unsigned __int128>(0x73F064D6D35442E1) << 64) | 0xB5D42F41915E5B82};

class IDIIr77Windows : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIIr77Windows> {
   public:
    IDIIr77Windows() {
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
        seat_shared_uuid<&GUIDIIr77Windows>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77Windows>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == GUIDIIr77Windows)
            obj = std::shared_ptr<IDIIr77Windows>(shared_from_this(), static_cast<IDIIr77Windows*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIIr77Windows)); }

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
        {"GUIDIIr77Windows", &GUIDIIr77Windows},
        {"GUIDIIr77Server", &GUIDIIr77Server},
    };
};

}  // namespace NSIr77Scene