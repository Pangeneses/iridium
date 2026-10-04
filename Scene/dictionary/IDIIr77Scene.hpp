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

inline Ir77GUID GUIDIIr77Scene{(static_cast<unsigned __int128>(0xFF3BBC89C52846C6) << 64) | 0xB7ABC31A43D1E78D};
inline Ir77GUID GUIDIIr77Mesh{(static_cast<unsigned __int128>(0x559950d115614816) << 64) | 0x8b5d9a80f77061bf};
inline Ir77GUID GUIDIIr77Light{(static_cast<unsigned __int128>(0x2b6d55d6cb7b4270) << 64) | 0xb3877cf6e50f2e42};
inline Ir77GUID GUIDIIr77Camera{(static_cast<unsigned __int128>(0xbbfc4c525bd04207) << 64) | 0x80a4b9735bbb388f};
inline Ir77GUID GUIDIIr77Material{(static_cast<unsigned __int128>(0x08c4e5614dde4682) << 64) | 0x9e00cc5509f3f1c1};
inline Ir77GUID GUIDIIr77Texture{(static_cast<unsigned __int128>(0x6174d72325344c0c) << 64) | 0x9874315cd8843c2a};
inline Ir77GUID GUIDIIr77Animation{(static_cast<unsigned __int128>(0x7cdff1fb4cde4616) << 64) | 0xb9c19437f8d1e740};

class IDIIr77Scene : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDIIr77Scene> {
   public:
    IDIIr77Scene() {
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
        seat_shared_uuid<&GUIDIIr77Scene>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77Scene>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == GUIDIIr77Scene)
            obj = std::shared_ptr<IDIIr77Scene>(shared_from_this(), static_cast<IDIIr77Scene*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIIr77Scene)); }

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
        {"GUIDIIr77Scene", &GUIDIIr77Scene},         {"GUIDIIr77Mesh", &GUIDIIr77Mesh},         {"GUIDIIr77Light", &GUIDIIr77Light},
        {"GUIDIIr77Camera", &GUIDIIr77Camera},       {"GUIDIIr77Material", &GUIDIIr77Material}, {"GUIDIIr77Texture", &GUIDIIr77Texture},
        {"GUIDIIr77Animation", &GUIDIIr77Animation},
    };
};

}  // namespace NSIr77Scene