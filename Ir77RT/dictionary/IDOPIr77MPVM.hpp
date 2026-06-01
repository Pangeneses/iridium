#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "IDIIr77MPVM.hpp"

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Dictionary.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Return.hpp"

#include "../runtime/Ir77GUID.hpp"
#include "../runtime/Ir77Return.hpp"

namespace NSIr77RT {

inline Ir77GUID GUIDOPIr77MPVM{(static_cast<unsigned __int128>(0x4246FB3759FB46A8) << 64) | 0x823643D14E94F739};

inline Ir77GUID GUIDOPIr77Object{(static_cast<unsigned __int128>(0xAE4843DA213445B7) << 64) | 0x96ADAE8728A8ED2D};
inline Ir77GUID GUIDOPIr77UUID{(static_cast<unsigned __int128>(0x1F3F278D6CE142B6) << 64) | 0x8B5FE81AFE3E5C81};
inline Ir77GUID GUIDOPIr77Boolean{(static_cast<unsigned __int128>(0xFFD69F47A97F4198) << 64) | 0xBC00743F174E0C89};
inline Ir77GUID GUIDOPIr77Char{(static_cast<unsigned __int128>(0x7E402B3B4D604CA7) << 64) | 0xB6CF95231172A30D};
inline Ir77GUID GUIDOPIr77WChar{(static_cast<unsigned __int128>(0x812949DDD7CD438B) << 64) | 0xB14157ACB54C9465};
inline Ir77GUID GUIDOPIr77String{(static_cast<unsigned __int128>(0xDEE8C34EC51E43EF) << 64) | 0xBDE6CD7E9B07EF33};
inline Ir77GUID GUIDOPIr77WString{(static_cast<unsigned __int128>(0x57F48E7932734BE3) << 64) | 0xA2EE2523B20088DD};
inline Ir77GUID GUIDOPIr77UInt8{(static_cast<unsigned __int128>(0x0360D10E742C4BAB) << 64) | 0xBF14A494FF044440};
inline Ir77GUID GUIDOPIr77UInt16{(static_cast<unsigned __int128>(0x3763A1D0BA9F4D23) << 64) | 0xB9FC6CDCB7EE7D46};
inline Ir77GUID GUIDOPIr77UInt32{(static_cast<unsigned __int128>(0x13A2A2C3C66746E1) << 64) | 0x84A4CE836CC2481C};
inline Ir77GUID GUIDOPIr77UInt64{(static_cast<unsigned __int128>(0x5E3EE09600A74487) << 64) | 0x91077D0A4A59EEB0};
inline Ir77GUID GUIDOPIr77Int16{(static_cast<unsigned __int128>(0x8810B86F741A4359) << 64) | 0x9D85BA3C580EEABB};
inline Ir77GUID GUIDOPIr77Int32{(static_cast<unsigned __int128>(0xE70909DDB79D4B9E) << 64) | 0x97A6B9B4B58294F2};
inline Ir77GUID GUIDOPIr77Int64{(static_cast<unsigned __int128>(0x57A92FCDA01944B0) << 64) | 0x9D4F7F899A1FD31E};
inline Ir77GUID GUIDOPIr77F32{(static_cast<unsigned __int128>(0x6453751EABF54F50) << 64) | 0xAACD1C43351A7EFA};
inline Ir77GUID GUIDOPIr77F64{(static_cast<unsigned __int128>(0x52DE133BC53F41FB) << 64) | 0xBC462E25B8C96653};
inline Ir77GUID GUIDOPIr77F128{(static_cast<unsigned __int128>(0xE27CB0FA4CE643DF) << 64) | 0xBDDB318B5C4E193C};
inline Ir77GUID GUIDOPIr77Data{(static_cast<unsigned __int128>(0xB8B214482DEB4A57) << 64) | 0x9F79E47D7EB063FC};
inline Ir77GUID GUIDOPIr77TimePoint{(static_cast<unsigned __int128>(0xACAE74B3D12E4519) << 64) | 0xB12B0460C783CDB6};
inline Ir77GUID GUIDOPIr77System{(static_cast<unsigned __int128>(0x05F2EA48E07944A2) << 64) | 0xA984D85934D43D49};
inline Ir77GUID GUIDOPIr77Tag{(static_cast<unsigned __int128>(0x511036B02ED54516) << 64) | 0x8758C6B605071CF8};
inline Ir77GUID GUIDOPIr77Item{(static_cast<unsigned __int128>(0x7AAC17CE9D754275) << 64) | 0xB3D4A78D27C06B76};
inline Ir77GUID GUIDOPIr77Collect{(static_cast<unsigned __int128>(0xA159C8C40E374037) << 64) | 0x8D6F5718B1F2AC31};
inline Ir77GUID GUIDOPIr77RetVar{(static_cast<unsigned __int128>(0x436FD47728ED4A97) << 64) | 0x80D34DCDCA5A5CC0};

class IDOPIr77MPVM : public IIr77Dictionary, public std::enable_shared_from_this<IDOPIr77MPVM> {
   public:
    IDOPIr77MPVM() {
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

    std::shared_ptr<IIr77Return const> EnlistedUuid(std::shared_ptr<IIr77GUID const>& uid) {
        uid.reset(reinterpret_cast<IIr77GUID const*>(&m_enlisted_uuid), [](auto*) {});
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnlistedChrono(std::chrono::system_clock::time_point& t) {
        t = m_enlisted;
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> Delist(std::shared_ptr<IIr77Return const>& condition) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

        m_valid = false;

        m_invalidation_condition = condition;

        m_delisted = std::chrono::system_clock::now();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> DelistedChrono(std::chrono::system_clock::time_point& t, std::shared_ptr<IIr77Return const>& condition) {
        t = m_delisted;

        condition = m_invalidation_condition;

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetSender(std::shared_ptr<IIr77Enlisted const>& sender) {
        m_sender = sender;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSender(std::shared_ptr<IIr77Enlisted const>& sender) const {
        sender = m_sender;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> SetSenderMsg(std::string const& msg) {
        m_message = msg;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> GetSenderMsg(std::string& msg) const {
        msg = m_message;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDOPIr77MPVM>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDOPIr77MPVM>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        if (iid == &GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == &GUIDOPIr77MPVM)
            obj = std::shared_ptr<IDOPIr77MPVM>(shared_from_this(), static_cast<IDOPIr77MPVM*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDOPIr77MPVM)); }

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
        {"GUIDOPIr77MPVM", &GUIDOPIr77MPVM},       {"GUIDOPIr77Object", &GUIDOPIr77Object}, {"GUIDOPIr77UUID", &GUIDOPIr77UUID},
        {"GUIDOPIr77Boolean", &GUIDOPIr77Boolean}, {"GUIDOPIr77Char", &GUIDOPIr77Char},     {"GUIDOPIr77WChar", &GUIDOPIr77WChar},
        {"GUIDOPIr77String", &GUIDOPIr77String},   {"GUIDOPIr77UInt8", &GUIDOPIr77UInt8},   {"GUIDOPIr77UInt16", &GUIDOPIr77UInt16},
        {"GUIDOPIr77UInt32", &GUIDOPIr77UInt32},   {"GUIDOPIr77UInt64", &GUIDOPIr77UInt64}, {"GUIDOPIr77Int16", &GUIDOPIr77Int16},
        {"GUIDOPIr77Int32", &GUIDOPIr77Int32},     {"GUIDOPIr77Int64", &GUIDOPIr77Int64},   {"GUIDOPIr77F32", &GUIDOPIr77F32},
        {"GUIDOPIr77F64", &GUIDOPIr77F64},         {"GUIDOPIr77F128", &GUIDOPIr77F128},     {"GUIDOPIr77Tag", &GUIDOPIr77Tag},
        {"GUIDOPIr77Collect", &GUIDOPIr77Collect}, {"GUIDOPIr77Item", &GUIDOPIr77Item},     {"GUIDOPIr77System", &GUIDOPIr77System},
        {"GUIDOPIr77RetVar", &GUIDOPIr77RetVar},
    };
};

}  // namespace NSIr77RT
