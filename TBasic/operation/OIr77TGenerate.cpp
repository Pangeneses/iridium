#include "OIr77TGenerate.hpp"
#include <memory>

#include "../../Ir77RT/dictionary/IDIIr77MPVM.hpp"

#include "../dictionary/IDIr77TBASIC.hpp"
#include "../dictionary/IDOIr77TBASIC.hpp"

namespace NSIr77TBasic {
OIr77TGenerate::OIr77TGenerate() {
    try {
        m_enlisted_uuid.Generate();
    } catch (std::invalid_argument a) {
        throw a;
    }

    m_enlisted = std::chrono::system_clock::now();

    m_operand.push_back(nullptr);
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::EnlistedAs(std::shared_ptr<IIr77GUID const>& uid) const {
    seat_shared_uuid<&GUIDIIr77Operand>(uid);

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::EnlistedUuid(std::shared_ptr<IIr77GUID const>& uid) const {
    uid.reset(reinterpret_cast<IIr77GUID const*>(&m_enlisted_uuid), [](auto*) {});

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::EnlistedChrono(std::chrono::system_clock::time_point& t) const {
    t = m_enlisted;

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::Delist(std::shared_ptr<IIr77Return const>& condition) {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    m_valid = false;

    m_invalidation_condition = condition;

    m_delisted = std::chrono::system_clock::now();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::DelistedChrono(std::chrono::system_clock::time_point& t,
                                                                  std::shared_ptr<IIr77Return const>& condition) const {
    t = m_delisted;

    condition = m_invalidation_condition;

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::SetSender(std::shared_ptr<IIr77Enlisted const>& sender) {
    m_sender = sender;

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::GetSender(std::shared_ptr<IIr77Enlisted const>& sender) const {
    sender = m_sender;

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::SetSenderMsg(std::string const& msg) {
    m_message = msg;

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::GetSenderMsg(std::string& msg) const {
    msg = m_message;

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::MemberOfUuid(std::shared_ptr<IIr77GUID const>& uid) const {
    seat_shared_uuid<&GUIDOIr77TGenerate>(uid);

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::CollectionUuid(std::shared_ptr<IIr77GUID const>& uid) const {
    seat_shared_uuid<&GUIDIr77TBASIC>(uid);

    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Enlisted has been invalidated.");

    return Ir77RETURN<Ir77OperationSucceeded>();
}

IIr77GUID* const OIr77TGenerate::QueryInterface(IIr77GUID const* iid, std::shared_ptr<void>& obj) {
    if (iid == &GUIDIIr77Enlisted)
        obj = std::shared_ptr<IIr77Enlisted>(shared_from_this(), static_cast<IIr77Enlisted*>(this));

    else if (iid == &GUIDIIr77Operand)
        obj = std::shared_ptr<IIr77Operand>(shared_from_this(), static_cast<IIr77Operand*>(this));

    else if (iid == &GUIDOIr77TGenerate)
        obj = std::shared_ptr<OIr77TGenerate>(shared_from_this(), static_cast<OIr77TGenerate*>(this));

    else
        return &GUIDQueryFailed;

    return &GUIDQuerySucceeded;
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::Resize(std::uint32_t const& sz) {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

    if (m_operand.size() - 1 < sz) return Ir77RETURN<Ir77InvalidArgument>(this, "Invalid arguement.");

    m_operand.resize(sz);

    for (std::shared_ptr<IIr77Enlisted const> enlisted : m_operand) {
        enlisted = nullptr;
    }

    return Ir77RETURN<Ir77InvalidOperation>(this, "Invalid operation; size 1.");
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::Size(std::uint32_t& sz) const {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

    sz = m_operand.size();

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::IsEmpty() const {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

    if (m_operand.size() == 0) {
        return Ir77RETURN<Ir77True>();
    } else {
        return Ir77RETURN<Ir77False>();
    }
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::SetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const> obj) {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

    if (obj == nullptr) return Ir77RETURN<Ir77InvalidArgument>(this, "Invalid arguement.");

    if (at > m_operand.size()) {
        return Ir77RETURN<Ir77InvalidArgument>(this, "Invalid arguement.");
    }

    m_operand.at(at) = obj;

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::GetIndexed(std::uint32_t const& at, std::shared_ptr<IIr77Enlisted const>& obj) const {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

    if (obj == nullptr) return Ir77RETURN<Ir77InvalidArgument>(this, "Invalid arguement.");

    if (at > m_operand.size()) {
        return Ir77RETURN<Ir77InvalidArgument>(this, "Invalid arguement.");
    }

    obj = m_operand.at(at);

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::SealOperand() {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    if (m_sealed) return Ir77RETURN<Ir77Sealed>(this, "Operand has been sealed.");

    m_sealed = true;

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::IsSealed() const {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    if (m_sealed) {
        return Ir77RETURN<Ir77True>();
    } else {
        return Ir77RETURN<Ir77False>();
    }
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::SetResult(std::shared_ptr<IIr77Operand const> const& outcome,
                                                             std::shared_ptr<IIr77Return const> const& ret) {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    if (m_complete) return Ir77RETURN<Ir77ProcessComplete>(this, "Operand process already complete.");

    if (ret == nullptr) return Ir77RETURN<Ir77InvalidArgument>(this, "Invalid arguement.");

    std::shared_ptr<IIr77GUID const> uid;

    if (outcome->MemberOfUuid(uid)->ID() == &GUIDIr77Invalidated) {
        return Ir77RETURN<Ir77Invalidated>(this, "Outcome is invalid.");
    }

    if (uid.get() != &GUIDIIr77Operand) return Ir77RETURN<Ir77InvalidArgument>(this, "Invalid operand.");

    m_complete = true;

    m_outcome = outcome;

    m_return = ret;

    return Ir77RETURN<Ir77OperationSucceeded>();
}

std::shared_ptr<IIr77Return const> OIr77TGenerate::GetResult(std::shared_ptr<IIr77Operand const>& outcome, std::shared_ptr<IIr77Return const>& ret) const {
    if (!m_valid) return Ir77RETURN<Ir77Invalidated>(this, "Operand has been invalidated.");

    if (m_complete) return Ir77RETURN<Ir77ProcessNotComplete>(this, "Operand process not complete.");

    outcome = m_outcome;

    ret = m_return;

    return Ir77RETURN<Ir77OperationSucceeded>();
}

}  // namespace NSIr77TBasic