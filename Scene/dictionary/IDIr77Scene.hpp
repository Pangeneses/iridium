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

namespace NSIr77Scene {

inline Ir77GUID GUIDIr77Scene{(static_cast<unsigned __int128>(0xabadfc86518e2664) << 64) | 0x09813be0211a4d9e};
inline Ir77GUID GUIDIr77Mesh{(static_cast<unsigned __int128>(0xaa985f8a3bbe5cf8) << 64) | 0x86c82e36a2794449};
inline Ir77GUID GUIDIr77Light{(static_cast<unsigned __int128>(0xba8d1aff87f08a38) << 64) | 0xf52443cbe3004579};
inline Ir77GUID GUIDIr77Camera{(static_cast<unsigned __int128>(0x867f03c5567dcaa8) << 64) | 0xe1523d005da94cc5};
inline Ir77GUID GUIDIr77Material{(static_cast<unsigned __int128>(0xb4b44680572eaf34) << 64) | 0xa469fa6d6bed42a3};
inline Ir77GUID GUIDIr77Texture{(static_cast<unsigned __int128>(0xa56a2add87f936e3) << 64) | 0x5bcb739b8ae04133};
inline Ir77GUID GUIDIr77Animation{(static_cast<unsigned __int128>(0xa9edd8cfc6758898) << 64) | 0x6dd8fb91b8c945af};

class IDIr77Scene : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIr77Scene> {
   public:
    IDIr77Scene() {
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
        seat_shared_uuid<&GUIDIr77Scene>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIr77Scene>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == GUIDIr77Scene)
            obj = std::shared_ptr<IDIr77Scene>(shared_from_this(), static_cast<IDIr77Scene*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIr77Scene)); }

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
        {"GUIDIr77Scene", &GUIDIr77Scene},         {"GUIDIr77Mesh", &GUIDIr77Mesh},         {"GUIDIr77Light", &GUIDIr77Light},
        {"GUIDIr77Camera", &GUIDIr77Camera},       {"GUIDIr77Material", &GUIDIr77Material}, {"GUIDIr77Texture", &GUIDIr77Texture},
        {"GUIDIr77Animation", &GUIDIr77Animation},
    };
};

}  // namespace NSIr77Scene