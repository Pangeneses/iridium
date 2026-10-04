#pragma once

#include <cstddef>
#include <string>
#include <chrono>
#include <memory>

#include "../dictionary/IDOPIr77MPVM.hpp"

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77MPVMOP.hpp"
#include "../interface/IIr77Return.hpp"

namespace NSIr77RT {

struct Ir77Object {
    using type = void*;
    static IIr77GUID* M_UUID() { return GUIDOPIr77Object; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77Object; };
};
struct Ir77UUID {
    using type = std::shared_ptr<IIr77GUID>;
    static IIr77GUID* M_UUID() { return GUIDOPIr77UUID; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77UUID; };
};
struct Ir77Boolean {
    using type = bool;
    static IIr77GUID* M_UUID() { return GUIDOPIr77Boolean; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77Boolean; };
};
struct Ir77Char {
    using type = char;
    static IIr77GUID* M_UUID() { return GUIDOPIr77Char; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77Boolean; };
};
struct Ir77WChar {
    using type = wchar_t;
    static IIr77GUID* M_UUID() { return GUIDOPIr77WChar; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77Boolean; };
};
struct Ir77String {
    using type = std::string;
    static IIr77GUID* M_UUID() { return GUIDOPIr77String; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77String; };
};
struct Ir77WString {
    using type = std::wstring;
    static IIr77GUID* M_UUID() { return GUIDOPIr77WString; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77WString; };
};
struct Ir77UInt8 {
    using type = std::uint8_t;
    static IIr77GUID* M_UUID() { return GUIDOPIr77UInt8; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77UInt8; };
};
struct Ir77UInt16 {
    using type = std::uint16_t;
    static IIr77GUID* M_UUID() { return GUIDOPIr77UInt16; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77UInt16; };
};
struct Ir77UInt32 {
    using type = std::uint32_t;
    static IIr77GUID* M_UUID() { return GUIDOPIr77UInt32; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77UInt32; };
};
struct Ir77UInt64 {
    using type = std::uint64_t;
    static IIr77GUID* M_UUID() { return GUIDOPIr77UInt64; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77UInt64; };
};
struct Ir77Int16 {
    using type = std::int16_t;
    static IIr77GUID* M_UUID() { return GUIDOPIr77Int16; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77Int16; };
};
struct Ir77Int32 {
    using type = std::int32_t;
    static IIr77GUID* M_UUID() { return GUIDOPIr77Int32; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77Int32; };
};
struct Ir77Int64 {
    using type = std::int64_t;
    static IIr77GUID* M_UUID() { return GUIDOPIr77Int64; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77Data; };
};
struct Ir77F32 {
    using type = float;
    static IIr77GUID* M_UUID() { return GUIDOPIr77F32; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77F32; };
};
struct Ir77F64 {
    using type = double;
    static IIr77GUID* M_UUID() { return GUIDOPIr77F64; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77F64; };
};
struct Ir77F128 {
    using type = long double;
    static IIr77GUID* M_UUID() { return GUIDOPIr77F64; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77F128; };
};
struct Ir77Data {
    using type = std::vector<std::byte>;
    static IIr77GUID* M_UUID() { return GUIDOPIr77Data; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77F128; };
};
struct Ir77TimePoint {
    using type = std::chrono::system_clock::time_point;
    static IIr77GUID* M_UUID() { return GUIDOPIr77F64; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77TimePoint; };
};
struct Ir77System {
    using type = std::shared_ptr<IIr77Enlisted const>;
    static IIr77GUID* M_UUID() { return GUIDOPIr77System; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77System; };
};
struct Ir77Tag {
    using type = struct {
        std::shared_ptr<IIr77GUID> uid;
        std::string path;
    };
    static IIr77GUID* M_UUID() { return GUIDOPIr77Tag; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77Tag; };
};
struct Ir77TaggedObject {
    using type = struct {
        Ir77Tag tag;
        std::shared_ptr<void> obj;
    };
    static IIr77GUID* M_UUID() { return GUIDOPIr77TaggedObject; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77TaggedObject; };
};
struct Ir77TaggedEnlisted {
    using type = struct {
        Ir77Tag tag;
        Ir77System system;
    };
    static IIr77GUID* M_UUID() { return GUIDOPIr77TaggedEnlisted; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77TaggedEnlisted; };
};
struct Ir77CollectObject {
    using type = std::vector<Ir77TaggedObject>;
    static IIr77GUID* M_UUID() { return GUIDOPIr77CollectObject; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77CollectObject; };
};
struct Ir77CollectEnlisted {
    using type = std::vector<Ir77TaggedEnlisted>;
    static IIr77GUID* M_UUID() { return GUIDOPIr77CollectEnlisted; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77CollectEnlisted; };
};
struct Ir77RetVar {
    using type = std::shared_ptr<IIr77Return const>;
    static IIr77GUID* M_UUID() { return GUIDOPIr77RetVar; };
    static Ir77MPVMOPType M_RAW() { return Ir77MPVMOPType::Ir77RetVar; };
};

template <typename T>
class Ir77MPVMOP : public IIr77Enlisted, public std::enable_shared_from_this<IIr77MPVMOP<T>> {
   public:
    Ir77MPVMOP() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    void Set(T::type in) { m_data = in; }

    T::type Get() { return m_data; }

    Ir77MPVMOPType RawType() { return T::M_RAW(); }

    T::type m_raw;

   public:
    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIIr77Operand>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnlistedUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        uid.reset(reinterpret_cast<IIr77GUID const*>(&m_enlisted_uuid), [](auto*) {});
        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnlistedChrono(std::chrono::system_clock::time_point& t) const {
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

    std::shared_ptr<IIr77Return const> DelistedChrono(std::chrono::system_clock::time_point& t, std::shared_ptr<IIr77Return const>& condition) const {
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

    std::shared_ptr<IIr77Return const> MemberUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<T::M_UUID()>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77RETURN>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    Ir77GUID QueryInterface(Ir77GUID iid, std::shared_ptr<void>& obj) {
        if (iid == GUIDIIr77Enlisted)
            obj = std::shared_ptr<IIr77Enlisted>(this->shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == GUIDOPIr77MPVM)
            obj = std::shared_ptr<IIr77MPVMOP<T>>(this->shared_from_this(), static_cast<IIr77MPVMOP<T>*>(this));

        else if (iid == &T::M_UUID())
            obj = std::shared_ptr<IIr77MPVMOP<T>>(this->shared_from_this(), static_cast<IIr77MPVMOP<T>*>(this));

        else
            return GUIDQueryFailed;

        return GUIDQuerySucceeded;
    }

    // ── Data ─────────────────────────────────────────────────────
   private:
    Ir77GUID m_enlisted_uuid{};

    std::chrono::system_clock::time_point m_enlisted{};

    std::chrono::system_clock::time_point m_delisted{};

    std::shared_ptr<IIr77Enlisted const> m_sender{nullptr};

    std::string m_message{""};

    bool m_valid{true};

    bool m_sealed{false};

    std::shared_ptr<IIr77Return const> m_invalidation_condition{nullptr};

    T::type m_data;
};

}  // namespace NSIr77RT