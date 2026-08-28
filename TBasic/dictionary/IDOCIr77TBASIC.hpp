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

namespace NSIr77TBasic {

inline Ir77GUID GUIDOCIr7TBASIC{(static_cast<unsigned __int128>(0xD1975E2CB0C548BF) << 64) | 0x9F00FC290B8E672A};

inline Ir77GUID GUIDOCIr77Assign{(static_cast<unsigned __int128>(0xB40C793D8C384EB6) << 64) | 0x853FEF6CF943E84F};
inline Ir77GUID GUIDOCIr77Get{(static_cast<unsigned __int128>(0x918D86B22E7F4E24) << 64) | 0x8EED88058470E433};
inline Ir77GUID GUIDOCIr77Equal{(static_cast<unsigned __int128>(0xB8F35B2953B946ED) << 64) | 0x85BEC4A141B929DE};
inline Ir77GUID GUIDOCIr77Not{(static_cast<unsigned __int128>(0x43253A5760D7407C) << 64) | 0xA2A7745FD0F99871};
inline Ir77GUID GUIDOCIr77Lesser{(static_cast<unsigned __int128>(0xD2E2578EEE594BD5) << 64) | 0x8C5C0AD60D5ADAC0};
inline Ir77GUID GUIDOCIr77Greater{(static_cast<unsigned __int128>(0xC5EB4F50BE7F4771) << 64) | 0xAF3CACBF11B13C47};
inline Ir77GUID GUIDOCIr77Generate{(static_cast<unsigned __int128>(0xBA2F4F37D0694064) << 64) | 0xB28B7AD52BECA35E};
inline Ir77GUID GUIDOCIr77Current{(static_cast<unsigned __int128>(0xEB0739E8F5D6471A) << 64) | 0x994E5C9FD6B7DAC5};

class IDOCIr7TBASIC : public Ir77Enlisted, public IIr77Dictionary, public std::enable_shared_from_this<IDOCIr7TBASIC> {
   public:
    IDOCIr7TBASIC() {
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
        seat_shared_uuid<&GUIDOCIr7TBASIC>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDOCIr7TBASIC>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDOCIr7TBASIC)
            obj = std::shared_ptr<IDOCIr7TBASIC>(shared_from_this(), static_cast<IDOCIr7TBASIC*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDOCIr7TBASIC)); }

    IIr77GUID const* Member(std::string const& member) const { return m_index.at(member); }

    std::map<std::string, IIr77GUID const*> const& Index() const { return m_index; }

   private:
    std::map<std::string, IIr77GUID const*> m_index{
        {"GUIDOCIr7TBASIC", &GUIDOCIr7TBASIC}, {"GUIDOCIr77Assign", &GUIDOCIr77Assign},     {"GUIDOCIr77Get", &GUIDOCIr77Get},
        {"GUIDOCIr77Equal", &GUIDOCIr77Equal}, {"GUIDOCIr77Lesser", &GUIDOCIr77Lesser},     {"GUIDOCIr77Greater", &GUIDOCIr77Greater},
        {"GUIDOCIr77Not", &GUIDOCIr77Not},     {"GUIDOCIr77Generate", &GUIDOCIr77Generate}, {"GUIDOCIr77Current", &GUIDOCIr77Current},
    };
};

}  // namespace NSIr77TBasic