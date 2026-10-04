#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Dictionary.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Return.hpp"

#include "../runtime/Ir77GUID.hpp"
#include "../runtime/Ir77Return.hpp"

namespace NSIr77RT {

inline Ir77GUID GUIDIIr77MPVM{(static_cast<unsigned __int128>(0x9347D36C009246A9) << 64) | 0x93ADB344A22135D2};

inline Ir77GUID GUIDIIr77Enlisted{(static_cast<unsigned __int128>(0xC052EA89334C43AE) << 64) | 0x9F976D7E354FC406};
inline Ir77GUID GUIDIIr77Dispatch{(static_cast<unsigned __int128>(0x53C09DF9D33C49A9) << 64) | 0x8DC2E12B514F380C};
inline Ir77GUID GUIDIIr77Patch{(static_cast<unsigned __int128>(0xFF068938AE404482) << 64) | 0x9A01953D49250A3F};
inline Ir77GUID GUIDIIr77Stack{(static_cast<unsigned __int128>(0x56358804686F4427) << 64) | 0x996533CAF29DD8D0};
inline Ir77GUID GUIDIIr77Operand{(static_cast<unsigned __int128>(0x1C6B89DA779A45D6) << 64) | 0x8F4DE47B675A62F0};
inline Ir77GUID GUIDIIr77Iterable{(static_cast<unsigned __int128>(0x4EA2C6BEEBFE4147) << 64) | 0x807BE59C8129E025};
inline Ir77GUID GUIDIIr77Iterator{(static_cast<unsigned __int128>(0x1EA673B27F98480A) << 64) | 0xB9E52CAD94206539};
inline Ir77GUID GUIDIIr77Dictionary{(static_cast<unsigned __int128>(0xEDA0BCD07ED54DE1) << 64) | 0xA97AD08DAA70563B};
inline Ir77GUID GUIDIIr77Return{(static_cast<unsigned __int128>(0x4B8F4936CB414575) << 64) | 0x8B58952FE375F7D7};

class IDIIr77MPVM : public IIr77Dictionary, public std::enable_shared_from_this<IDIIr77MPVM> {
   public:
    IDIIr77MPVM() {
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

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77MPVM>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) {
        seat_shared_uuid<&GUIDIIr77MPVM>(uid);
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDIIr77Dictionary)
            obj = std::shared_ptr<IIr77Dictionary>(shared_from_this(), static_cast<IIr77Dictionary*>(this));

        else if (iid == GUIDIIr77MPVM)
            obj = std::shared_ptr<IDIIr77MPVM>(shared_from_this(), static_cast<IDIIr77MPVM*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // ── IIr77Dictionary ──────────────────────────────────────────
   public:
    IIr77GUID const* Collection() const { return const_cast<IIr77GUID*>(static_cast<IIr77GUID const*>(&GUIDIIr77MPVM)); }

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
        {"GUIDIIr77MPVM", &GUIDIIr77MPVM},         {"GUIDIIr77Enlisted", &GUIDIIr77Enlisted}, {"GUIDIIr77Dispatch", &GUIDIIr77Dispatch},
        {"GUIDIIr77Patch", &GUIDIIr77Patch},       {"GUIDIIr77Stack", &GUIDIIr77Stack},       {"GUIDIIr77Iterable", &GUIDIIr77Iterable},
        {"GUIDIIr77Iterator", &GUIDIIr77Iterator}, {"GUIDIIr77Operand", &GUIDIIr77Operand},   {"GUIDIIr77Dictionary", &GUIDIIr77Dictionary},
        {"GUIDIIr77Return", &GUIDIIr77Return},
    };
};

}  // namespace NSIr77RT