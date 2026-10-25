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

// ---- collection ---------------------------------------------------------------------------------------------------------------------------------
inline Ir77GUID GUIDIIr77Scene{(static_cast<unsigned __int128>(0xFF3BBC89C52846C6) << 64) | 0xB7ABC31A43D1E78D};

// ---- server -------------------------------------------------------------------------------------------------------------------------------------
inline Ir77GUID GUIDIIr77Server{(static_cast<unsigned __int128>(0x96E5B37600A04548) << 64) | 0x94D2234479629D4D};

// ---- runtime -- Animate coordinates its peers ---------------------------------------------------------------------------------------------------
inline Ir77GUID GUIDIIr77Animate{(static_cast<unsigned __int128>(0xDDCC1637E91D4094) << 64) | 0x8FF28A0358B7731E};
inline Ir77GUID GUIDIIr77Pose{(static_cast<unsigned __int128>(0x308F2E5EBF9F4F59) << 64) | 0xBC93710E13FB13DF};
inline Ir77GUID GUIDIIr77View{(static_cast<unsigned __int128>(0xF932E0D060D54E7F) << 64) | 0xB3845F2DBB828B49};
inline Ir77GUID GUIDIIr77Lighting{(static_cast<unsigned __int128>(0x78D08C0AC3324EFE) << 64) | 0x931E67E715BE0977};
inline Ir77GUID GUIDIIr77Field{(static_cast<unsigned __int128>(0x1022E4384ABA49F8) << 64) | 0xAA273901BDB12A9F};

// ---- glTF loader --------------------------------------------------------------------------------------------------------------------------------
inline Ir77GUID GUIDIIr77Mesh{(static_cast<unsigned __int128>(0x559950D115614816) << 64) | 0x8B5D9A80F77061BF};
inline Ir77GUID GUIDIIr77Light{(static_cast<unsigned __int128>(0x2B6D55D6CB7B4270) << 64) | 0xB3877CF6E50F2E42};
inline Ir77GUID GUIDIIr77Camera{(static_cast<unsigned __int128>(0xBBFC4C525BD04207) << 64) | 0x80A4B9735BBB388F};
inline Ir77GUID GUIDIIr77Material{(static_cast<unsigned __int128>(0x08C4E5614DDE4682) << 64) | 0x9E00CC5509F3F1C1};
inline Ir77GUID GUIDIIr77Texture{(static_cast<unsigned __int128>(0x6174D72325344C0C) << 64) | 0x9874315CD8843C2A};
inline Ir77GUID GUIDIIr77Animation{(static_cast<unsigned __int128>(0x7CDFF1FB4CDE4616) << 64) | 0xB9C19437F8D1E740};

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

    std::shared_ptr<IIr77Enlisted const> m_sender{};

    std::string m_message{};

    bool m_valid{true};

    std::shared_ptr<IIr77Return const> m_invalidation_condition{};

    // same order as the declarations above
    std::map<std::string, IIr77GUID const*> m_index{
        {"GUIDIIr77Scene", &GUIDIIr77Scene},         {"GUIDIIr77Server", &GUIDIIr77Server},     {"GUIDIIr77Animate", &GUIDIIr77Animate},
        {"GUIDIIr77Pose", &GUIDIIr77Pose},           {"GUIDIIr77View", &GUIDIIr77View},         {"GUIDIIr77Lighting", &GUIDIIr77Lighting},
        {"GUIDIIr77Field", &GUIDIIr77Field},         {"GUIDIIr77Mesh", &GUIDIIr77Mesh},         {"GUIDIIr77Light", &GUIDIIr77Light},
        {"GUIDIIr77Camera", &GUIDIIr77Camera},       {"GUIDIIr77Material", &GUIDIIr77Material}, {"GUIDIIr77Texture", &GUIDIIr77Texture},
        {"GUIDIIr77Animation", &GUIDIIr77Animation},
    };
};

}  // namespace NSIr77Scene