#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../../Ir77RT/interface/IIr77GUID.hpp"
#include "../../Ir77RT/interface/IIr77Dictionary.hpp"
#include "../../Ir77RT/interface/IIr77Enlisted.hpp"
#include "../../Ir77RT/interface/IIr77Return.hpp"

#include "../../Ir77RT/runtime/Ir77GUID.hpp"
#include "../../Ir77RT/runtime/Ir77Return.hpp"

using namespace NSIr77RT;

namespace NSIr77TBasic {

inline Ir77GUID GUIDOPIr77TBasic{(static_cast<unsigned __int128>(0x1C397D82EE854881) << 64) | 0xAED39158F73E3651};

inline Ir77GUID GUIDOPIr77AlphaNumeric{(static_cast<unsigned __int128>(0x5F36D7A1288C485C) << 64) | 0xA6BD78CFF56A9559};
inline Ir77GUID GUIDOPIr77Numeric{(static_cast<unsigned __int128>(0x927E91C4F7434333) << 64) | 0x91AD3F64CA0DF89C};
inline Ir77GUID GUIDOPIr77UID{(static_cast<unsigned __int128>(0xFF69FE1367044D3F) << 64) | 0x83247A2AF26C1FB6};
inline Ir77GUID GUIDOPIr77MMM{(static_cast<unsigned __int128>(0x5B1E1631C0F44480) << 64) | 0xA93DDA2A6FBE334E};
inline Ir77GUID GUIDOPIr77CHRONO{(static_cast<unsigned __int128>(0x8A7C05A5DED24A80) << 64) | 0xB9AB5E16989326B5};
inline Ir77GUID GUIDOPIr77MMDDYYYY{(static_cast<unsigned __int128>(0xDF47E1E16EE6405F) << 64) | 0x93F0AB8FC4291904};
inline Ir77GUID GUIDOPIr77HHMMSS{(static_cast<unsigned __int128>(0xA5BEB89E8A1446DB) << 64) | 0x8788E698D459E6AC};
inline Ir77GUID GUIDOPIr77MaxPath{(static_cast<unsigned __int128>(0x79F4BEC1230D4721) << 64) | 0xB714A285D86ABF45};
inline Ir77GUID GUIDOPIr77SysID{(static_cast<unsigned __int128>(0x6CBB26F17C7A4F91) << 64) | 0x946F8A764764717C};
inline Ir77GUID GUIDOPIr77Hex{(static_cast<unsigned __int128>(0x95E8C95A02FB41D2) << 64) | 0x8D7B451AF2C2F722};
inline Ir77GUID GUIDOPIr77TextBlobA{(static_cast<unsigned __int128>(0xB48B773CE5074C67) << 64) | 0x87C1166C7032E4D1};
inline Ir77GUID GUIDOPIr77TextBlobW{(static_cast<unsigned __int128>(0xB06B2D4E4E49433E) << 64) | 0x929626970B18DCA8};

class IDOPIr77TBASIC : public IIr77Dictionary, public std::enable_shared_from_this<IDOPIr77TBASIC> {
   public:
    IDOPIr77TBASIC() {
        m_enlisted = std::chrono::system_clock::now();

        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }
    }

    // ── IIr77Enlisted ────────────────────────────────────────────
   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77Dictionary>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDOPIr77TBasic>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDOPIr77TBasic>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDOPIr77TBasic)
            obj = std::shared_ptr<IDOPIr77TBASIC>(shared_from_this(), static_cast<IDOPIr77TBASIC*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDOPIr77TBasic)); }

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
        {"GUIDOPIr77TBasic", &GUIDOPIr77TBasic},
        {"GUIDOPIr77AlphaNumeric", &GUIDOPIr77AlphaNumeric},
        {"GUIDOPIr77Numeric", &GUIDOPIr77Numeric},
        {"GUIDOPIr77UID", &GUIDOPIr77UID},
        {"GUIDOPIr77MMM", &GUIDOPIr77MMM},
        {"GUIDOPIr77CHRONO", &GUIDOPIr77CHRONO},
        {"GUIDOPIr77MMDDYYYY", &GUIDOPIr77MMDDYYYY},
        {"GUIDOPIr77HHMMSS", &GUIDOPIr77HHMMSS},
        {"GUIDOPIr77MaxPath", &GUIDOPIr77MaxPath},
        {"GUIDOPIr77SysID", &GUIDOPIr77SysID},
        {"GUIDOPIr77Hex", &GUIDOPIr77Hex},
        {"GUIDOPIr77TextBlobA", &GUIDOPIr77TextBlobA},
        {"GUIDOPIr77TextBlobW", &GUIDOPIr77TextBlobW},
    };
};

}  // namespace NSIr77TBasic
