#pragma once

#include <chrono>
#include <memory>

#include "../dictionary/IDIr77RET.hpp"

#include "../interface/IIr77GUID.hpp"
#include "../interface/IIr77Enlisted.hpp"
#include "../interface/IIr77Return.hpp"

#include "../runtime/Ir77RetLog.hpp"

namespace NSIr77RT {

typedef enum class Ir77RETEnum : unsigned int {
    EIr77RET,
    EIr77Unknown = 1,
    EIr77False = 2,
    EIr77True = 3,
    EIr77IsEqual = 4,
    EIr77IsLesser = 5,
    EIr77IsGreater = 6,
    EIr77InvalidArgument = 7,
    EIr77ValidArgument = 8,
    EIr77OperationFailed = 9,
    EIr77OperationConflict = 10,
    EIr77OperatorSucceededed = 11,
    EIr77InvalidOperation = 12,
    EIr77NotConfigured = 13,
    EIr77AlreadyConfigured = 14,
    EIr77ImproperlyConfigedured = 15,
    EIr77ProperlyConfigured = 16,
    EIr77MPVMError = 17,
    EIr77OutOfMemory = 18,
    EIr77OutOfRange = 19,
    EIr77KeyNotFound = 20,
    EIr77InvalidPointer = 21,
    EIr77InvalidInterface = 22,
    EIr77InvalidToken = 23,
    EIr77Empty = 24,
    EIr77Dirty = 25,
    EIr77Sealed = 26,
    EIr77Invalidated = 27,
    EIr77NotSystemModule = 28,
    EIr77ModuleNotFound = 29,
    EIr77RtclassNotFound = 30,
    EIr77ProcessNotFound = 31,
    EIr77ProcessNotCompletete = 32,
    EIr77ProcessComplete = 33,
    EIr77ImageNotLoaded = 34,
    EIr77MPVMFailed = 35,
    EIr77MPVMSucceeded = 36,
    EIr77DirDoesntExist = 37,
    EIr77DirAlreadyExists = 38,
    EIr77PageAlreadyExists = 39,
    EIr77NoFileOpen = 40,
    EIr77FileDoesntExist = 41,
    EIr77FileAlreadyExists = 42,
    EIr77FileHandleOpen = 43,
    EIr77FileBadFormatting = 44
} Ir77RETEnum;

struct Ir77Unknown {
    static IIr77GUID* MemberUUID() { return &GUIDIr77Unknown; }
};
struct Ir77False {
    static IIr77GUID* MemberUUID() { return &GUIDIr77False; }
};
struct Ir77True {
    static IIr77GUID* MemberUUID() { return &GUIDIr77True; }
};
struct Ir77IsEqual {
    static IIr77GUID* MemberUUID() { return &GUIDIr77IsEqual; }
};
struct Ir77IsLesser {
    static IIr77GUID* MemberUUID() { return &GUIDIr77IsLesser; }
};
struct Ir77IsGreater {
    static IIr77GUID* MemberUUID() { return &GUIDIr77IsGreater; }
};
struct Ir77InvalidArgument {
    static IIr77GUID* MemberUUID() { return &GUIDIr77InvalidArgument; }
};
struct Ir77ValidArgument {
    static IIr77GUID* MemberUUID() { return &GUIDIr77ValidArgument; }
};
struct Ir77OperationFailed {
    static IIr77GUID* MemberUUID() { return &GUIDIr77OperationFailed; }
};
struct Ir77OperationConflict {
    static IIr77GUID* MemberUUID() { return &GUIDIr77OperationConflict; }
};
struct Ir77OperationSucceeded {
    static IIr77GUID* MemberUUID() { return &GUIDIr77OperationSucceeded; }
};
struct Ir77InvalidOperation {
    static IIr77GUID* MemberUUID() { return &GUIDIr77InvalidOperation; }
};
struct Ir77NotConfigured {
    static IIr77GUID* MemberUUID() { return &GUIDIr77NotConfigured; }
};
struct Ir77AlreadyConfigure {
    static IIr77GUID* MemberUUID() { return &GUIDIr77AlreadyConfigure; }
};
struct Ir77ImproperlyConfiged {
    static IIr77GUID* MemberUUID() { return &GUIDIr77ImproperlyConfiged; }
};
struct Ir77ProperlyConfigured {
    static IIr77GUID* MemberUUID() { return &GUIDIr77ProperlyConfigured; }
};
struct Ir77MPVMError {
    static IIr77GUID* MemberUUID() { return &GUIDIr77MPVMError; }
};
struct Ir77OutOfMemory {
    static IIr77GUID* MemberUUID() { return &GUIDIr77OutOfMemory; }
};
struct Ir77OutOfRange {
    static IIr77GUID* MemberUUID() { return &GUIDIr77OutOfRange; }
};
struct Ir77KeyNotFound {
    static IIr77GUID* MemberUUID() { return &GUIDIr77KeyNotFound; }
};
struct Ir77InvalidPointer {
    static IIr77GUID* MemberUUID() { return &GUIDIr77InvalidPointer; }
};
struct Ir77InvalidInterface {
    static IIr77GUID* MemberUUID() { return &GUIDIr77InvalidInterface; }
};
struct Ir77InvalidToken {
    static IIr77GUID* MemberUUID() { return &GUIDIr77InvalidToken; }
};
struct Ir77Empty {
    static IIr77GUID* MemberUUID() { return &GUIDIr77Empty; }
};
struct Ir77Dirty {
    static IIr77GUID* MemberUUID() { return &GUIDIr77Dirty; }
};
struct Ir77Sealed {
    static IIr77GUID* MemberUUID() { return &GUIDIr77Sealed; }
};
struct Ir77Invalidated {
    static IIr77GUID* MemberUUID() { return &GUIDIr77Invalidated; }
};
struct Ir77NotSystemModule {
    static IIr77GUID* MemberUUID() { return &GUIDIr77NotSystemModule; }
};
struct Ir77ModuleNotFound {
    static IIr77GUID* MemberUUID() { return &GUIDIr77ModuleNotFound; }
};
struct Ir77RTClassNotFound {
    static IIr77GUID* MemberUUID() { return &GUIDIr77RTClassNotFound; }
};
struct Ir77ProcessNotFound {
    static IIr77GUID* MemberUUID() { return &GUIDIr77ProcessNotFound; }
};
struct Ir77ProcessNotComplete {
    static IIr77GUID* MemberUUID() { return &GUIDIr77ProcessNotComplete; }
};
struct Ir77ProcessComplete {
    static IIr77GUID* MemberUUID() { return &GUIDIr77ProcessComplete; }
};
struct Ir77ImageNotLoaded {
    static IIr77GUID* MemberUUID() { return &GUIDIr77ImageNotLoaded; }
};
struct Ir77MPVMFailed {
    static IIr77GUID* MemberUUID() { return &GUIDIr77MPVMFailed; }
};
struct Ir77MPVMSucceeded {
    static IIr77GUID* MemberUUID() { return &GUIDIr77MPVMSucceeded; }
};
struct Ir77DirDoesntExist {
    static IIr77GUID* MemberUUID() { return &GUIDIr77DirDoesntExist; }
};
struct Ir77DirAlreadyExists {
    static IIr77GUID* MemberUUID() { return &GUIDIr77DirAlreadyExists; }
};
struct Ir77PageAlreadyExist {
    static IIr77GUID* MemberUUID() { return &GUIDIr77PageAlreadyExist; }
};
struct Ir77NoFileOpen {
    static IIr77GUID* MemberUUID() { return &GUIDIr77NoFileOpen; }
};
struct Ir77FileDoesntExist {
    static IIr77GUID* MemberUUID() { return &GUIDIr77FileDoesntExist; }
};
struct Ir77FileAlreadyExist {
    static IIr77GUID* MemberUUID() { return &GUIDIr77FileAlreadyExist; }
};
struct Ir77FileHandleOpen {
    static IIr77GUID* MemberUUID() { return &GUIDIr77FileHandleOpen; }
};
struct Ir77FileBadFormatting {
    static IIr77GUID* MemberUUID() { return &GUIDIr77FileBadFormatting; }
};

inline static std::string g_empty_log = "";

inline static IIr77Enlisted const* g_null_sender = nullptr;

template <typename T>
class Ir77Return;

template <typename T>
std::shared_ptr<IIr77Return const> Ir77RETURN(IIr77Enlisted const* sender = g_null_sender, std::string const& message = g_empty_log) {
    std::shared_ptr<Ir77Return<T>> shared = std::make_shared<Ir77Return<T>>();

    if (sender) {
        std::shared_ptr<void> obj;
        Ir77GUID enlisted_id{(static_cast<unsigned __int128>(0xC052EA89334C43AE) << 64) | 0x9F976D7E354FC406};
        const_cast<IIr77Enlisted*>(sender)->QueryInterface(&enlisted_id, obj);
        if (obj) {
            shared->PublicSetSender(std::static_pointer_cast<IIr77Enlisted const>(obj));
        }
    }

    if (!message.empty()) Ir77RetLog::Write(message);

    shared->PublicSetMsg(message);

    return shared;
}

template <typename T>
class Ir77Return : public IIr77Return, public std::enable_shared_from_this<Ir77Return<T>> {
   public:
    friend std::shared_ptr<IIr77Return const> Ir77RETURN(IIr77Enlisted const*, std::string const&);

   public:
    Ir77Return() {
        try {
            m_enlisted_uuid.Generate();
        } catch (std::invalid_argument a) {
            throw a;
        }

        m_enlisted = std::chrono::system_clock::now();
    }

    std::shared_ptr<IIr77Return const> EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
        // seat_shared_uuid<&GUIDIIr77Enlisted>(uid);

        uid = nullptr;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnlistedUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        uid.reset(reinterpret_cast<IIr77GUID const*>(&m_enlisted_uuid), [](auto*) {});

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> EnlistedChrono(std::chrono::system_clock::time_point& t) const {
        t = m_enlisted;

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

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

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

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
        uid.reset(T::MemberUUID(), [](auto*) {});

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
        seat_shared_uuid<&GUIDIr77RETURN>(uid);

        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    IIr77GUID* const QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
        Ir77GUID enlisted_id{(static_cast<unsigned __int128>(0xC052EA89334C43AE) << 64) | 0x9F976D7E354FC406};
        Ir77GUID return_id{(static_cast<unsigned __int128>(0x4B8F4936CB414575) << 64) | 0x8B58952FE375F7D7};

        if (iid == &enlisted_id)
            obj = std::shared_ptr<IIr77Enlisted>(this->shared_from_this(), static_cast<IIr77Enlisted*>(this));

        else if (iid == &return_id)
            obj = std::shared_ptr<IIr77Return>(this->shared_from_this(), static_cast<IIr77Return*>(this));

        else if (iid == T::MemberUUID())
            obj = std::shared_ptr<Ir77Return<T>>(this->shared_from_this(), static_cast<Ir77Return<T>*>(this));

        else
            return &GUIDQueryFailed;

        return &GUIDQuerySucceeded;
    }

    IIr77GUID const* ID() const { return T::MemberUUID(); }

    IIr77GUID const* GID() const { return &GUIDIr77RETURN; }

    std::shared_ptr<IIr77Return const> InvalidateReturn(std::shared_ptr<IIr77Return const>& condition) {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

        m_valid = false;

        m_invalidation_condition = condition;

        m_delisted = std::chrono::system_clock::now();

        return Ir77RETURN<Ir77OperationSucceeded>();
    }

    std::shared_ptr<IIr77Return const> IsInvalid() const {
        if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Invalidated.");

        return Ir77RETURN<Ir77True>();
    }

    std::shared_ptr<IIr77Return const> IsInvalid(std::shared_ptr<IIr77Return const>& condition) {
        if (!m_valid)
            return Ir77RETURN<Ir77Invalidated>(this, "Invalidated.");

        else
            condition = m_invalidation_condition;

        return Ir77RETURN<Ir77True>();
    }

   public:
    void PublicSetSender(std::shared_ptr<IIr77Enlisted const> sender) { m_sender = sender; }

    void PublicSetMsg(std::string const& msg) { m_message = msg; }

    // ── Data ─────────────────────────────────────────────────────
   private:
    Ir77GUID m_enlisted_uuid{};

    std::chrono::system_clock::time_point m_enlisted{};

    std::chrono::system_clock::time_point m_delisted{};

    std::shared_ptr<IIr77Enlisted const> m_sender{nullptr};

    std::string m_message{""};

    bool m_valid{true};

    std::shared_ptr<IIr77Return const> m_invalidation_condition{nullptr};
};

}  // namespace NSIr77RT